/*
 * Tencent is pleased to support the open source community by making PhoenixGo available.
 * 
 * Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
 * 
 * Licensed under the BSD 3-Clause License (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     https://opensource.org/licenses/BSD-3-Clause
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "async_dist_zero_model_client.h"

#include <future>

#include <glog/logging.h>

AsyncDistZeroModelClient::AsyncDistZeroModelClient(const std::vector<std::string> &svr_addrs,
                                                   const DistConfig &dist_config)
    : m_config(dist_config),
      m_svr_addrs(svr_addrs),
      m_bucket_mutexes(new std::mutex[svr_addrs.size()])
{
    CHECK(!m_svr_addrs.empty());
    for (size_t i = 0; i < m_svr_addrs.size(); ++i) {
        m_stubs.emplace_back(DistZeroModel::NewStub(
                grpc::CreateChannel(m_svr_addrs[i], grpc::InsecureChannelCredentials())));
        m_avail_stubs.push(i);
        m_leaky_buckets.emplace_back(m_config.leaky_bucket_size(), m_config.leaky_bucket_refill_period_ms());
    }
    m_forward_rpc_complete_thread = std::thread(&AsyncRpcQueue::Complete, &m_forward_rpc_queue, -1);
}

AsyncDistZeroModelClient::~AsyncDistZeroModelClient()
{
    m_forward_rpc_queue.Shutdown();
    LOG(INFO) << "~AsyncDistZeroModelClient waiting async rpc complete thread stop";
    m_forward_rpc_complete_thread.join();
    LOG(INFO) << "~AsyncDistZeroModelClient waiting all stubs released";
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]{ return m_avail_stubs.size() == m_stubs.size(); });
    LOG(INFO) << "~AsyncDistZeroModelClient succ";
}

int AsyncDistZeroModelClient::Init(const ModelConfig &model_config)
{
    AsyncRpcQueue queue;
    InitReq req;
    req.mutable_model_config()->CopyFrom(model_config);
    int ret = 0;
    for (size_t i = 0; i < m_stubs.size(); ++i) {
        queue.Call<InitReq, InitResp>(
            BindAsyncRpcFunc(*m_stubs[i], AsyncInit), req,
            [this, i, &ret](grpc::Status &status, InitResp &resp) {
                if (!status.ok()) {
                    LOG(ERROR) << "DistZeroModel::Init error, " << m_svr_addrs[i] << ", ret "
                               << status.error_code() << ": " << status.error_message();
                    ret = status.error_code();
                }
            }
        );
    }
    queue.Complete(m_stubs.size());
    return ret;
}

int AsyncDistZeroModelClient::GetGlobalStep(int &global_step)
{
    AsyncRpcQueue queue;
    GetGlobalStepReq req;
    int ret = 0;
    std::vector<int> global_steps(m_stubs.size());
    for (size_t i = 0; i < m_stubs.size(); ++i) {
        queue.Call<GetGlobalStepReq, GetGlobalStepResp>(
            BindAsyncRpcFunc(*m_stubs[i], AsyncGetGlobalStep), req,
            [this, i, &ret, &global_steps](grpc::Status &status, GetGlobalStepResp &resp) {
                if (status.ok()) {
                    global_steps[i] = resp.global_step();
                } else {
                    LOG(ERROR) << "DistZeroModel::GetGlobalStep error, " << m_svr_addrs[i] << ", ret "
                               << status.error_code() << ": " << status.error_message();
                    ret = status.error_code();
                }
            }
        );
    }
    queue.Complete(m_stubs.size());
    if (ret == 0) {
        for (size_t i = 1; i < m_stubs.size(); ++i) {
            if (global_steps[i] != global_steps[0]) {
                LOG(ERROR) << "Recived different global_step, "
                           << global_steps[i] << "(" << m_svr_addrs[i] << ")" << " vs "
                           << global_steps[0] << "(" << m_svr_addrs[0] << ")";
                ret = ERR_GLOBAL_STEP_CONFLICT;
            }
        }
    }
    if (ret == 0) {
        global_step = global_steps[0];
    }
    return ret;
}

void AsyncDistZeroModelClient::Forward(const std::vector<std::vector<bool>> &inputs, callback_t callback)
{
    int stub_id = GetStub();
    ForwardReq req;
    for (const auto &features: inputs) {
        if (features.size() != INPUT_DIM) {
            LOG(ERROR) << "Error input dim not match, need " << INPUT_DIM << ", got " << features.size();
            callback(ERR_INVALID_INPUT, {}, {});
            return;
        }
        std::string encode_features((INPUT_DIM + 7) / 8, 0);
        for (int i = 0; i < INPUT_DIM; ++i) {
            encode_features[i / 8] |= (unsigned char)features[i] << (i % 8);
        }
        req.add_inputs(encode_features);
    }
    m_forward_rpc_queue.Call<ForwardReq, ForwardResp>(
        BindAsyncRpcFunc(*m_stubs[stub_id], AsyncForward), req,
        [this, stub_id, callback](grpc::Status &status, ForwardResp &resp) {
            if (status.ok() && resp.outputs_size() == 0) {
                status = grpc::Status(grpc::StatusCode(ERR_EMPTY_RESP), "receive empty response");
            }

            bool release_now = true;
            if (!status.ok()) {
                m_stubs[stub_id] = DistZeroModel::NewStub(
                    grpc::CreateChannel(m_svr_addrs[stub_id], grpc::InsecureChannelCredentials()));
                if (m_config.enable_leaky_bucket()) {
                    std::lock_guard<std::mutex> lock(m_bucket_mutexes[stub_id]);
                    m_leaky_buckets[stub_id].ConsumeToken();
                    release_now = !m_leaky_buckets[stub_id].Empty();
                }
            }
            if (release_now) {
                ReleaseStub(stub_id);
            } else {
                DisableStub(stub_id);
            }

            if (status.ok()) {
                std::vector<std::vector<float>> policy;
                std::vector<float> value;
                for (auto &output: resp.outputs()) {
                    policy.emplace_back(output.policy().begin(), output.policy().end());
                    value.push_back(output.value());
                }
                callback(0, std::move(policy), std::move(value));
            } else if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
                LOG(ERROR) << "DistZeroModel::Forward timeout, " << m_svr_addrs[stub_id];
                callback(ERR_FORWARD_TIMEOUT, {}, {});
            } else {
                LOG(ERROR) << "DistZeroModel::Forward error, " << m_svr_addrs[stub_id] << " "
                           << status.error_code() << ": " << status.error_message();
                callback(status.error_code(), {}, {});
            }
        },
        m_config.timeout_ms()
    );
}

int AsyncDistZeroModelClient::Forward(const std::vector<std::vector<bool>> &inputs,
                                      std::vector<std::vector<float>> &policy, std::vector<float> &value)
{
    std::promise<std::tuple<int, std::vector<std::vector<float>>, std::vector<float>>> promise;
    Forward(inputs, [&promise](int ret, std::vector<std::vector<float>> policy, std::vector<float> value) {
        promise.set_value(std::make_tuple(ret, std::move(policy), std::move(value)));
    });
    int ret;
    std::tie(ret, policy, value) = promise.get_future().get();
    return ret;
}

int AsyncDistZeroModelClient::RpcQueueSize()
{
    return m_forward_rpc_queue.Size();
}

void AsyncDistZeroModelClient::Wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]{ return !m_avail_stubs.empty(); });
}

int AsyncDistZeroModelClient::GetStub()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]{ return !m_avail_stubs.empty(); });
    int stub_id = m_avail_stubs.top();
    m_avail_stubs.pop();
    return stub_id;
}

void AsyncDistZeroModelClient::ReleaseStub(int stub_id)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_avail_stubs.push(stub_id);
    }
    m_cond.notify_one();
}

void AsyncDistZeroModelClient::DisableStub(int stub_id)
{
    LOG(ERROR) << "disable DistZeroModel " << m_svr_addrs[stub_id];
    std::thread([this, stub_id]() {
        {
            std::lock_guard<std::mutex> lock(m_bucket_mutexes[stub_id]);
            m_leaky_buckets[stub_id].WaitRefill();
        }
        ReleaseStub(stub_id);
        LOG(INFO) << "reenable DistZeroModel " << m_svr_addrs[stub_id];
    }).detach();
}

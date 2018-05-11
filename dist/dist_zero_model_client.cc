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
#include "dist_zero_model_client.h"

#include <thread>

#include <glog/logging.h>
#include <grpc++/grpc++.h>

DistZeroModelClient::DistZeroModelClient(const std::string &server_address, const DistConfig &dist_config)
    : m_config(dist_config),
      m_server_address(server_address),
      m_stub(DistZeroModel::NewStub(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()))),
      m_leaky_bucket(dist_config.leaky_bucket_size(), dist_config.leaky_bucket_refill_period_ms())
{
}

int DistZeroModelClient::Init(const ModelConfig &model_config)
{
    InitReq req;
    InitResp resp;

    req.mutable_model_config()->CopyFrom(model_config);

    grpc::ClientContext context;
    grpc::Status status = m_stub->Init(&context, req, &resp);

    if (status.ok()) {
        return 0;
    } else {
        LOG(ERROR) << "DistZeroModel::Init error, " << m_server_address << ", ret "
                   << status.error_code() << ": " << status.error_message();
        return status.error_code();
    }
}

int DistZeroModelClient::GetGlobalStep(int &global_step)
{
    GetGlobalStepReq req;
    GetGlobalStepResp resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->GetGlobalStep(&context, req, &resp);

    if (status.ok()) {
        global_step = resp.global_step();
        return 0;
    } else {
        LOG(ERROR) << "DistZeroModel::GetGlobalStep error, " << m_server_address << ", ret "
                   << status.error_code() << ": " << status.error_message();
        return status.error_code();
    }
}

int DistZeroModelClient::Forward(const std::vector<std::vector<bool>>& inputs,
                                 std::vector<std::vector<float>> &policy, std::vector<float> &value)
{
    ForwardReq req;
    ForwardResp resp;

    for (const auto &features: inputs) {
        if (features.size() != INPUT_DIM) {
            LOG(ERROR) << "Error input dim not match, need " << INPUT_DIM << ", got " << features.size();
            return ERR_INVALID_INPUT;
        }
        std::string encode_features((INPUT_DIM + 7) / 8, 0);
        for (int i = 0; i < INPUT_DIM; ++i) {
            encode_features[i / 8] |= (unsigned char)features[i] << (i % 8);
        }
        req.add_inputs(encode_features);
    }

    grpc::ClientContext context;
    if (m_config.timeout_ms() > 0) {
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(m_config.timeout_ms()));
    }
    grpc::Status status = m_stub->Forward(&context, req, &resp);

    if (!status.ok()) {
        m_stub = DistZeroModel::NewStub(grpc::CreateChannel(m_server_address, grpc::InsecureChannelCredentials()));
        if (m_config.enable_leaky_bucket()) {
            m_leaky_bucket.ConsumeToken();
        }
    }

    if (status.ok()) {
        policy.clear();
        value.clear();
        for (auto &output: resp.outputs()) {
            policy.emplace_back(output.policy().begin(), output.policy().end());
            value.push_back(output.value());
        }
        return 0;
    } else if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
        LOG(ERROR) << "DistZeroModel::Forward timeout, " << m_server_address;
        return ERR_FORWARD_TIMEOUT;
    } else {
        LOG(ERROR) << "DistZeroModel::Forward error, " << m_server_address << " "
                   << status.error_code() << ": " << status.error_message();
        return status.error_code();
    }
}

void DistZeroModelClient::Wait()
{
    if (m_config.enable_leaky_bucket() && m_leaky_bucket.Empty()) {
        LOG(ERROR) << "disable DistZeroModel " << m_server_address;
        m_leaky_bucket.WaitRefill();
        LOG(INFO) << "reenable DistZeroModel " << m_server_address;
    }
}

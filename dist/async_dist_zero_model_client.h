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
#pragma once

#include <memory>
#include <stack>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "model/zero_model_base.h"

#include "dist/async_rpc_queue.h"
#include "dist/leaky_bucket.h"
#include "dist/dist_config.pb.h"
#include "dist/dist_zero_model.grpc.pb.h"

class AsyncDistZeroModelClient final : public ZeroModelBase
{
 public:
    AsyncDistZeroModelClient(const std::vector<std::string> &svr_addrs, const DistConfig &dist_config);

    ~AsyncDistZeroModelClient() override;

    int Init(const ModelConfig &model_config) override;

    int Forward(const std::vector<std::vector<bool>>& inputs,
                std::vector<std::vector<float>> &policy, std::vector<float> &value) override;

    void Forward(const std::vector<std::vector<bool>> &inputs, callback_t callback) override;

    int GetGlobalStep(int &global_step) override;

    int RpcQueueSize() override;

    void Wait() override;

 private:
    int GetStub();

    void ReleaseStub(int stub_id);

    void DisableStub(int stub_id);

 private:
    DistConfig m_config;
    std::vector<std::string> m_svr_addrs;
    std::vector<std::unique_ptr<DistZeroModel::Stub>> m_stubs;
    AsyncRpcQueue m_forward_rpc_queue;
    std::thread m_forward_rpc_complete_thread;

    std::stack<int> m_avail_stubs;
    std::mutex m_mutex;
    std::condition_variable m_cond;

    std::vector<LeakyBucket> m_leaky_buckets;
    std::unique_ptr<std::mutex[]> m_bucket_mutexes;
};

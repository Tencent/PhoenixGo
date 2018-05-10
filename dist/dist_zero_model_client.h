/*
 * Tencent is pleased to support the open source community by making Phoenix Go available.
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

#include "model/zero_model_base.h"

#include "dist/leaky_bucket.h"
#include "dist/dist_config.pb.h"
#include "dist/dist_zero_model.grpc.pb.h"

class DistZeroModelClient final : public ZeroModelBase
{
 public:
    DistZeroModelClient(const std::string &server_adress, const DistConfig &dist_config);

    int Init(const ModelConfig &model_config) override;

    int Forward(const std::vector<std::vector<bool>>& inputs,
                std::vector<std::vector<float>> &policy, std::vector<float> &value) override;

    int GetGlobalStep(int &global_step) override;

    void Wait() override;

 private:
    DistConfig m_config;
    std::string m_server_address;
    std::unique_ptr<DistZeroModel::Stub> m_stub;
    LeakyBucket m_leaky_bucket;
};

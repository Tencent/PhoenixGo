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

#include <vector>
#include <functional>

#include "common/errordef.h"

#include "model/model_config.pb.h"

class ZeroModelBase
{
 public:
    typedef std::function<void(int, std::vector<std::vector<float>>, std::vector<float>)> callback_t;

    virtual ~ZeroModelBase() {}

    virtual int Init(const ModelConfig &model_config) = 0;

    virtual int Forward(const std::vector<std::vector<bool>> &inputs,
                        std::vector<std::vector<float>> &policy, std::vector<float> &value) = 0;

    virtual void Forward(const std::vector<std::vector<bool>> &inputs, callback_t callback)
    {
        std::vector<std::vector<float>> policy;
        std::vector<float> value;
        int ret = Forward(inputs, policy, value);
        callback(ret, std::move(policy), std::move(value));
    }

    virtual int GetGlobalStep(int &global_step) = 0;

    virtual int RpcQueueSize() { return 0; }

    virtual void Wait() {}

    enum {
        INPUT_DIM  = 19 * 19 * 17,
        OUTPUT_DIM = 19 * 19 + 1,
    };
};

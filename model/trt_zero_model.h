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

#include "model/zero_model_base.h"
#include "model/model_config.pb.h"

namespace nvinfer1 {
    class ICudaEngine;
    class IRuntime;
    class IExecutionContext;
}

class TrtZeroModel final : public ZeroModelBase
{
 public:
    TrtZeroModel(int gpu);
    ~TrtZeroModel();

    int Init(const ModelConfig &model_config) override;

    // input  [batch, 19 * 19 * 17]
    // policy [batch, 19 * 19 + 1]
    int Forward(const std::vector<std::vector<bool>> &inputs,
                std::vector<std::vector<float>> &policy, std::vector<float> &value) override;

    int GetGlobalStep(int &global_step) override;

 private:
    nvinfer1::ICudaEngine *m_engine;
    nvinfer1::IRuntime *m_runtime;
    nvinfer1::IExecutionContext *m_context;
    std::vector<void*> m_cuda_buf;
    int m_gpu;
    int m_global_step;
};

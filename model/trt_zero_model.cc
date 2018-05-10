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
#include "trt_zero_model.h"

#if GOOGLE_TENSORRT

#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <glog/logging.h>

#include "cuda/include/cuda_runtime_api.h"
#include "tensorrt/include/NvInfer.h"

namespace fs = boost::filesystem;

class Logger : public nvinfer1::ILogger
{
    void log(Severity severity, const char *msg) override
    {
        switch (severity) {
         case Severity::kINTERNAL_ERROR: LOG(ERROR) << msg; break;
         case Severity::kERROR: LOG(ERROR) << msg; break;
         case Severity::kWARNING: LOG(WARNING) << msg; break;
         case Severity::kINFO: LOG(INFO) << msg; break;
        }
    }
} g_logger;

TrtZeroModel::TrtZeroModel(int gpu)
    : m_engine(nullptr), m_runtime(nullptr), m_context(nullptr), m_gpu(gpu), m_global_step(0)
{
}

TrtZeroModel::~TrtZeroModel()
{
    if (m_context) {
        m_context->destroy();
    }
    if (m_engine) {
        m_engine->destroy();
    }
    if (m_runtime) {
        m_runtime->destroy();
    }
    for (auto buf: m_cuda_buf) {
        int ret = cudaFree(buf);
        if (ret != 0) {
            LOG(ERROR) << "cuda free err " << ret;
        }
    }
}

int TrtZeroModel::Init(const ModelConfig &model_config)
{
    cudaSetDevice(m_gpu);

    fs::path train_dir = model_config.train_dir();

    fs::path tensorrt_model_path = model_config.tensorrt_model_path();
    if (tensorrt_model_path.is_relative()) {
        tensorrt_model_path = train_dir / tensorrt_model_path;
    }

    std::ostringstream model_ss(std::ios::binary);
    if (!(model_ss << std::ifstream(tensorrt_model_path.string(), std::ios::binary).rdbuf())) {
        PLOG(ERROR) << "read tensorrt model '" << tensorrt_model_path << "' error";
        return ERR_READ_TRT_MODEL;
    }
    std::string model_str = model_ss.str();

    m_runtime = nvinfer1::createInferRuntime(g_logger);
    m_engine = m_runtime->deserializeCudaEngine(model_str.c_str(), model_str.size(), nullptr);
    if (m_engine == nullptr) {
        PLOG(ERROR) << "load cuda engine error";
        return ERR_LOAD_TRT_ENGINE;
    }
    m_context = m_engine->createExecutionContext();

    int batch_size = m_engine->getMaxBatchSize();
    LOG(INFO) << "tensorrt max batch size: " << batch_size;
    for (int i = 0; i < m_engine->getNbBindings(); ++i) {
        auto dim = m_engine->getBindingDimensions(i);
        std::string dim_str = "(";
        int size = 1;
        for (int i = 0; i < dim.nbDims; ++i) {
            if (i) dim_str += ", ";
            dim_str += std::to_string(dim.d[i]);
            size *= dim.d[i];
        }
        dim_str += ")";
        LOG(INFO) << "tensorrt binding: " << m_engine->getBindingName(i) << " " << dim_str;

        void *buf;
        int ret = cudaMalloc(&buf, batch_size * size * sizeof(float));
        if (ret != 0) {
            LOG(ERROR) << "cuda malloc err " << ret;
            return ERR_CUDA_MALLOC;
        }
        m_cuda_buf.push_back(buf);
    }

    if (!(std::ifstream(tensorrt_model_path.string() + ".step") >> m_global_step)) {
        LOG(WARNING) << "read global step from " << tensorrt_model_path << ".step failed";
    }

    return 0;
}

int TrtZeroModel::Forward(const std::vector<std::vector<bool>> &inputs,
                          std::vector<std::vector<float>> &policy, std::vector<float> &value)
{
    int batch_size = inputs.size();
    if (batch_size == 0) {
        LOG(ERROR) << "Error batch size can not be 0.";
        return ERR_INVALID_INPUT;
    }

    std::vector<float> inputs_flat(batch_size * INPUT_DIM);
    for (int i = 0; i < batch_size; ++i) {
        if (inputs[i].size() != INPUT_DIM) {
            LOG(ERROR) << "Error input dim not match, need " << INPUT_DIM << ", got " << inputs[i].size();
            return ERR_INVALID_INPUT;
        }
        for (int j = 0; j < INPUT_DIM; ++j) {
            inputs_flat[i * INPUT_DIM + j] = inputs[i][j];
        }
    }

    int ret = cudaMemcpy(m_cuda_buf[0], inputs_flat.data(), inputs_flat.size() * sizeof(float), cudaMemcpyHostToDevice);
    if (ret != 0) {
        LOG(ERROR) << "cuda memcpy err " << ret;
        return ERR_CUDA_MEMCPY;
    }

    m_context->execute(batch_size, m_cuda_buf.data());

    std::vector<float> policy_flat(batch_size * OUTPUT_DIM);
    ret = cudaMemcpy(policy_flat.data(), m_cuda_buf[1], policy_flat.size() * sizeof(float), cudaMemcpyDeviceToHost);
    if (ret != 0) {
        LOG(ERROR) << "cuda memcpy err " << ret;
        return ERR_CUDA_MEMCPY;
    }
    policy.resize(batch_size);
    for (int i = 0; i < batch_size; ++i) {
        policy[i].resize(OUTPUT_DIM);
        for (int j = 0; j < OUTPUT_DIM; ++j) {
            policy[i][j] = policy_flat[i * OUTPUT_DIM + j];
        }
    }

    value.resize(batch_size);
    ret = cudaMemcpy(value.data(), m_cuda_buf[2], value.size() * sizeof(float), cudaMemcpyDeviceToHost);
    if (ret != 0) {
        LOG(ERROR) << "cuda memcpy err " << ret;
        return ERR_CUDA_MEMCPY;
    }
    for (int i = 0; i < batch_size; ++i) {
        value[i] = -value[i];
    }

    return 0;
}

int TrtZeroModel::GetGlobalStep(int &global_step)
{
    global_step = m_global_step;
    return 0;
}

#else // GOOGLE_TENSORRT

#include <glog/logging.h>

TrtZeroModel::TrtZeroModel(int gpu)
{
    LOG(FATAL) << "TensorRT is not enable!";
}

TrtZeroModel::~TrtZeroModel()
{
    LOG(FATAL) << "TensorRT is not enable!";
}

int TrtZeroModel::Init(const ModelConfig &model_config)
{
    LOG(FATAL) << "TensorRT is not enable!";
    return 0;
}

int TrtZeroModel::Forward(const std::vector<std::vector<bool>> &inputs,
                          std::vector<std::vector<float>> &policy, std::vector<float> &value)
{
    LOG(FATAL) << "TensorRT is not enable!";
    return 0;
}

int TrtZeroModel::GetGlobalStep(int &global_step)
{
    LOG(FATAL) << "TensorRT is not enable!";
    return 0;
}

#endif // GOOGLE_TENSORRT

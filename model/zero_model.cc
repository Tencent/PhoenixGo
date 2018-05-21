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
#include "zero_model.h"

#include <string>

#include <glog/logging.h>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/protobuf/meta_graph.pb.h"

#include "model/checkpoint_utils.h"

namespace fs = boost::filesystem;
namespace tf = tensorflow;

const std::string input_tensor_name  = "inputs";
const std::string policy_tensor_name = "policy";
const std::string value_tensor_name  = "value";

ZeroModel::ZeroModel(int gpu)
    : m_session(nullptr), m_gpu(gpu)
{
}

ZeroModel::~ZeroModel()
{
    if (m_session != nullptr) {
        tf::Status status = m_session->Close();
        if (!status.ok()) {
            LOG(ERROR) << "Error closing tf session: " << status.ToString();
        }
    }
}

int ZeroModel::Init(const ModelConfig &model_config)
{
    fs::path train_dir = model_config.train_dir();

    fs::path meta_graph_path = model_config.meta_graph_path();
    if (meta_graph_path.empty()) {
        meta_graph_path = train_dir / "meta_graph";
    } else if (meta_graph_path.is_relative()) {
        meta_graph_path = train_dir / meta_graph_path;
    }

    fs::path checkpoint_path = model_config.checkpoint_path();
    if (checkpoint_path.empty()) {
        checkpoint_path = GetCheckpointPath(train_dir);
    } else if (checkpoint_path.is_relative()) {
        checkpoint_path = train_dir / checkpoint_path;
    }

    if (checkpoint_path.empty()) {
        return ERR_READ_CHECKPOINT;
    }
    LOG(INFO) << "Read checkpoint state succ";

    tf::MetaGraphDef meta_graph_def;
    tf::Status status = ReadBinaryProto(tf::Env::Default(), meta_graph_path.string(), &meta_graph_def);
    if (!status.ok()) {
        LOG(ERROR) << "Error reading graph definition from " << meta_graph_path << ": " << status.ToString();
        return ERR_READ_CHECKPOINT;
    }
    LOG(INFO) << "Read meta graph succ";

    for (auto &node: *meta_graph_def.mutable_graph_def()->mutable_node()) {
        node.set_device("/gpu:" + std::to_string(m_gpu));
    }

    tf::SessionOptions options;
    options.config.set_allow_soft_placement(true);
    options.config.mutable_gpu_options()->set_per_process_gpu_memory_fraction(0.5);
    options.config.mutable_gpu_options()->set_allow_growth(true);
    options.config.set_intra_op_parallelism_threads(model_config.intra_op_parallelism_threads());
    options.config.set_inter_op_parallelism_threads(model_config.inter_op_parallelism_threads());
    if (model_config.enable_xla()) {
        options.config.mutable_graph_options()->mutable_optimizer_options()->set_global_jit_level(tf::OptimizerOptions::ON_1);
    }
    m_session = std::unique_ptr<tf::Session>(tf::NewSession(options));
    if (m_session == nullptr) {
        LOG(ERROR) << "Could not create Tensorflow session.";
        return ERR_CREATE_SESSION;
    }
    LOG(INFO) << "Create session succ";

    status = m_session->Create(meta_graph_def.graph_def());
    if (!status.ok()) {
        LOG(ERROR) << "Error creating graph: " << status.ToString();
        return ERR_CREATE_GRAPH;
    }
    LOG(INFO) << "Create graph succ";

    tf::Tensor checkpoint_path_tensor(tf::DT_STRING, tf::TensorShape());
    checkpoint_path_tensor.scalar<std::string>()() = checkpoint_path.string();
    status = m_session->Run({{meta_graph_def.saver_def().filename_tensor_name(), checkpoint_path_tensor}},
                            {}, /* fetches_outputs is empty */
                            {meta_graph_def.saver_def().restore_op_name()},
                            nullptr);
    if (!status.ok()) {
        LOG(ERROR) << "Error loading checkpoint from " << checkpoint_path << ": " << status.ToString();
        return ERR_RESTORE_VAR;
    }
    LOG(INFO) << "Load checkpoint succ";

    std::vector<std::vector<bool>> inputs(1, std::vector<bool>(INPUT_DIM, false));
    std::vector<std::vector<float>> policy;
    std::vector<float> value;
    Forward(inputs, policy, value);

    return 0;
}

int ZeroModel::Forward(const std::vector<std::vector<bool>> &inputs,
                       std::vector<std::vector<float>> &policy, std::vector<float> &value)
{
    int batch_size = inputs.size();
    if (batch_size == 0) {
        LOG(ERROR) << "Error batch size can not be 0.";
        return ERR_INVALID_INPUT;
    }

    tf::Tensor feature_tensor(tf::DT_BOOL, tf::TensorShape({batch_size, INPUT_DIM}));
    auto matrix = feature_tensor.matrix<bool>();
    for (int i = 0; i < batch_size; ++i) {
        if (inputs[i].size() != INPUT_DIM) {
            LOG(ERROR) << "Error input dim not match, need " << INPUT_DIM << ", got " << inputs[i].size();
            return ERR_INVALID_INPUT;
        }
        for (int j = 0; j < INPUT_DIM; ++j) {
            matrix(i, j) = inputs[i][j];
        }
    }


    std::vector<std::pair<std::string, tf::Tensor>> network_inputs = {{input_tensor_name, feature_tensor}};
    std::vector<std::string> fetch_outputs = {policy_tensor_name, value_tensor_name};
    std::vector<tf::Tensor> network_outputs;
    tf::Status status = m_session->Run(network_inputs, fetch_outputs, {}, &network_outputs);
    if (!status.ok()) {
        LOG(ERROR) << "Error session run: " << status.ToString();
        return ERR_SESSION_RUN;
    }

    auto policy_tensor = network_outputs[0].matrix<float>();
    auto value_tensor  = network_outputs[1].flat<float>();
    policy.resize(batch_size);
    value.resize(batch_size);
    for (int i = 0; i < batch_size; ++i) {
        policy[i].resize(OUTPUT_DIM);
        for (int j = 0; j < OUTPUT_DIM; ++j) {
            policy[i][j] = policy_tensor(i, j);
        }
        value[i] = -value_tensor(i);
    }

    return 0;
}

int ZeroModel::GetGlobalStep(int &global_step)
{
    std::vector<tf::Tensor> network_outputs;
    tf::Status status = m_session->Run({}, {"global_step"}, {}, &network_outputs);
    if (!status.ok()) {
        LOG(ERROR) << "Error session run: " << status.ToString();
        return ERR_SESSION_RUN;
    }

    global_step = network_outputs[0].scalar<int64_t>()();
    return 0;
}

void ZeroModel::SetMKLEnv(const ModelConfig &model_config)
{
#if defined(_WIN32) || defined(_WIN64)
    _putenv_s("KMP_BLOCKTIME", std::to_string(model_config.kmp_blocktime()).c_str());
    _putenv_s("KMP_SETTINGS", std::to_string(model_config.kmp_settings()).c_str());
    _putenv_s("KMP_AFFINITY", model_config.kmp_affinity().c_str());
    if (model_config.intra_op_parallelism_threads() > 0) {
        _putenv_s("OMP_NUM_THREADS", std::to_string(model_config.intra_op_parallelism_threads()).c_str());
    }
#else
    setenv("KMP_BLOCKTIME", std::to_string(model_config.kmp_blocktime()).c_str(), 0);
    setenv("KMP_SETTINGS", std::to_string(model_config.kmp_settings()).c_str(), 0);
    setenv("KMP_AFFINITY", model_config.kmp_affinity().c_str(), 0);
    if (model_config.intra_op_parallelism_threads() > 0) {
        setenv("OMP_NUM_THREADS", std::to_string(model_config.intra_op_parallelism_threads()).c_str(), 0);
    }
#endif
}

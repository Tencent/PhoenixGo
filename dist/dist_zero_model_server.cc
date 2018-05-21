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
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <grpc++/grpc++.h>

#include "common/timer.h"
#include "model/zero_model.h"
#include "model/trt_zero_model.h"

#include "dist/dist_zero_model.grpc.pb.h"

DEFINE_string(server_address, "", "Server address.");
DEFINE_int32(gpu, 0, "Use which gpu.");

class DistZeroModelServiceImpl final : public DistZeroModel::Service
{
 public:
    grpc::Status Init(grpc::ServerContext *context, const InitReq *req, InitResp *resp) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        LOG(INFO) << "Init with config: " << req->model_config().DebugString();

        if (req->model_config().enable_mkl()) {
            ZeroModel::SetMKLEnv(req->model_config());
        }

        m_model.reset(new ZeroModel(FLAGS_gpu));
        if (req->model_config().enable_tensorrt()) {
            m_model.reset(new TrtZeroModel(FLAGS_gpu));
        }

        int ret = m_model->Init(req->model_config());
        if (ret == 0) {
            LOG(INFO) << "Init model succ";
            return grpc::Status::OK;
        } else {
            LOG(ERROR) << "Init model error: " << ret;
            return grpc::Status(grpc::StatusCode(ret), "Init model error");
        }
    }

    grpc::Status GetGlobalStep(grpc::ServerContext *context,
                               const GetGlobalStepReq *req, GetGlobalStepResp *resp) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_model == nullptr) {
            return grpc::Status(grpc::StatusCode(-1), "DistZeroModel hasn't init");
        }

        int global_step;
        int ret = m_model->GetGlobalStep(global_step);

        if (ret == 0) {
            LOG(INFO) << "Get global_step=" << global_step;
            resp->set_global_step(global_step);
            return grpc::Status::OK;
        } else {
            LOG(INFO) << "Get global_step error: " << ret;
            return grpc::Status(grpc::StatusCode(ret), "Get global_step error");
        }
    }

    grpc::Status Forward(grpc::ServerContext *context, const ForwardReq *req, ForwardResp *resp) override
    {
        Timer timer;
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_model == nullptr) {
            return grpc::Status(grpc::StatusCode(-1), "DistZeroModel hasn't init");
        }

        std::vector<std::vector<bool>> inputs;
        for (const auto &encode_features: req->inputs()) {
            if ((int)encode_features.size() * 8 < m_model->INPUT_DIM) {
                LOG(ERROR) << "Error input features need " << m_model->INPUT_DIM << " bits, recv only "
                           << encode_features.size() * 8;
                return grpc::Status(grpc::StatusCode(ERR_INVALID_INPUT), "Forward error");
            }
            std::vector<bool> features(m_model->INPUT_DIM);
            for (int i = 0; i < m_model->INPUT_DIM; ++i) {
                features[i] = (unsigned char)encode_features[i / 8] >> (i % 8) & 1;
            }
            inputs.push_back(std::move(features));
        }

        std::vector<std::vector<float>> policy;
        std::vector<float> value;
        int ret = m_model->Forward(inputs, policy, value);

        if (ret == 0) {
            for (size_t i = 0; i < policy.size(); ++i) {
                auto *output = resp->add_outputs();
                for (const auto &p: policy[i]) {
                    output->add_policy(p);
                }
                output->set_value(value[i]);
            }
            if (resp->outputs_size() == 0) LOG(ERROR) << "input batch size " << inputs.size() << ", output 0!!!";
            LOG_EVERY_N(INFO, 1000) << "Forward succ.";
            return grpc::Status::OK;
        } else {
            LOG(ERROR) << "Forward error: " << ret;
            return grpc::Status(grpc::StatusCode(ret), "Forward error");
        }
    }

 private:
    std::unique_ptr<ZeroModelBase> m_model;
    std::mutex m_mutex;
};

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    DistZeroModelServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(FLAGS_server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOG(INFO) << "Server listening on " << FLAGS_server_address;
    server->Wait();
}

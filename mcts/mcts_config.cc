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
#include "mcts_config.h"

#include <fstream>

#include <google/protobuf/text_format.h>
#include <glog/logging.h>

std::unique_ptr<MCTSConfig> LoadConfig(const char *config_path)
{
    auto config = std::unique_ptr<MCTSConfig>(new MCTSConfig);
    std::ostringstream conf_ss;
    if (!(conf_ss << std::ifstream(config_path).rdbuf())) {
        PLOG(ERROR) << "read config file " << config_path << " error";
        return nullptr;
    }
    if (!google::protobuf::TextFormat::ParseFromString(conf_ss.str(), config.get())) {
        LOG(ERROR) << "parse config file " << config_path << " error! buf=" << conf_ss.str();
        return nullptr;
    }
    return config;
}

std::unique_ptr<MCTSConfig> LoadConfig(const std::string &config_path)
{
    return LoadConfig(config_path.c_str());
}

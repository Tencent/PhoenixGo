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
#include "checkpoint_utils.h"

#include <glog/logging.h>
#include <google/protobuf/text_format.h>

#include "model/checkpoint_state.pb.h"

namespace fs = boost::filesystem;

fs::path GetCheckpointPath(const fs::path &train_dir)
{
    tensorflow::CheckpointState checkpoint_state;
    fs::path ckpt_state_path = train_dir / "checkpoint";
    std::ostringstream ckpt_ss;
    if (!(ckpt_ss << std::ifstream(ckpt_state_path.string()).rdbuf())) {
        PLOG(ERROR) << "Error reading " << ckpt_state_path;
        return "";
    }
    if (!google::protobuf::TextFormat::ParseFromString(ckpt_ss.str(), &checkpoint_state)) {
        LOG(ERROR) << "Error parsing " << ckpt_state_path << ", buf=" << ckpt_ss.str();
        return "";
    }
    fs::path checkpoint_path = checkpoint_state.model_checkpoint_path();
    if (checkpoint_path.is_relative()) {
        checkpoint_path = train_dir / checkpoint_path;
    }
    return checkpoint_path;
}

bool CopyCheckpoint(const fs::path &from, const fs::path &to)
{
    for (int i = 0; i < 3; ++i) {
        try {
            fs::path from_ckpt_path = GetCheckpointPath(from);
            if (from_ckpt_path.empty()) {
                LOG(ERROR) << "Error reading model path from " << from;
                continue;
            }

            fs::path ckpt_name = from_ckpt_path.filename();
            fs::path to_ckpt_path = to / ckpt_name;

            fs::create_directories(to);
            for (std::string suffix: {".data-00000-of-00001", ".index"}) {
                fs::path from_file_path = from_ckpt_path.string() + suffix;
                fs::path to_file_path = to_ckpt_path.string() + suffix;
                LOG(INFO) << "Copying from " << from_file_path << " to " << to_file_path << suffix;
                fs::copy_file(from_file_path, to_file_path, fs::copy_option::overwrite_if_exists);
                fs::path symlink_path = to / ("zero.ckpt" + suffix);
                fs::remove(symlink_path);
                fs::create_symlink(to_file_path.filename(), symlink_path);
            }

            tensorflow::CheckpointState checkpoint_state;
            checkpoint_state.set_model_checkpoint_path(to_ckpt_path.string());
            fs::path ckpt_state_path = to / "checkpoint";
            LOG(INFO) << "Writing checkpoint file " << ckpt_state_path;
            if (!(std::ofstream(ckpt_state_path.string()) << checkpoint_state.DebugString())) {
                PLOG(ERROR) << "Error writing " << to << "/checkpoint";
                continue;
            }

            fs::copy_file(from / "meta_graph", to / "meta_graph", fs::copy_option::overwrite_if_exists);
            LOG(INFO) << "Copy checkpoint from " << from << " to " << to << " succ.";
            return true;
        } catch (const std::exception &e) {
            LOG(ERROR) << e.what();
        }
    }
    return false;
}

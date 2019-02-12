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

#include <string>

class MCTSEngine;
class MCTSDebugger
{
 public:
    MCTSDebugger(MCTSEngine *engine);

    void Debug(); // call before move

    std::string GetDebugStr();
    std::string GetLastMoveDebugStr(); // call after move
    void UpdateLastMoveDebugStr();

    std::string GetMainMovePath(int rank);
    void PrintTree(int depth, int topk, const std::string &prefix = "");

 private:
    MCTSEngine *m_engine;
    std::string m_last_move_debug_str;
};

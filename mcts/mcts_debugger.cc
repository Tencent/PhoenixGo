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
#include "mcts_debugger.h"

#include <queue>
#include <utility>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>

#include <glog/logging.h>

#include "common/go_comm.h"

#include "mcts_engine.h"

MCTSDebugger::MCTSDebugger(MCTSEngine *engine)
    : m_engine(engine)
{
}

void MCTSDebugger::Debug() // call before move
{
    if (VLOG_IS_ON(1)) {
        int ith = m_engine->m_num_moves + 1;
        std::string ith_str = std::to_string(ith) + "th move(" + "wb"[ith&1] + ")";
        VLOG(1) << "========== debug info for " << ith_str << " begin ==========";
        VLOG(1) << "main move path: " << GetMainMovePath(0);
        VLOG(1) << "second move path: " << GetMainMovePath(1);
        VLOG(1) << "third move path: " << GetMainMovePath(2);
        int depth = m_engine->GetConfig().debugger().print_tree_depth();
        int width = m_engine->GetConfig().debugger().print_tree_width();
        PrintTree(depth ? depth : 1, width ? width : 10);
        VLOG(1) << "model global step: " << m_engine->m_model_global_step;
        VLOG(1) << "========== debug info for " << ith_str << " end   ==========";
    }
}

std::string MCTSDebugger::GetDebugStr()
{
    TreeNode *root = m_engine->m_root;
    int ith = m_engine->m_num_moves;
    std::string ith_str = std::to_string(ith) + "th move(" + "wb"[ith&1] + ")";
    float root_action = (float)root->total_action / k_action_value_base / root->visit_count;
    std::string debug_str =
        ith_str + ": " + GoFunction::IdToStr(root->move) +
        ", winrate=" + std::to_string((root_action + 1) * 50) + "%" +
        ", N=" + std::to_string(root->visit_count) +
        ", Q=" + std::to_string(root_action) +
        ", p=" + std::to_string(root->prior_prob) +
        ", v=" + std::to_string(root->value);
    if (m_engine->m_simulation_counter > 0) {
        debug_str +=
            ", cost " + std::to_string(m_engine->m_search_timer.fms()) + "ms" +
            ", sims=" + std::to_string(m_engine->m_simulation_counter) +
            ", height=" + std::to_string(m_engine->m_monitor.MaxSearchTreeHeight()) +
            ", avg_height=" + std::to_string(m_engine->m_monitor.AvgSearchTreeHeight());
    }
    debug_str += ", global_step=" + std::to_string(m_engine->m_model_global_step);
    return debug_str;
}

std::string MCTSDebugger::GetLastMoveDebugStr()
{
    return m_last_move_debug_str;
}

void MCTSDebugger::UpdateLastMoveDebugStr()
{
    m_last_move_debug_str = GetDebugStr();
}

std::string MCTSDebugger::GetMainMovePath(int rank)
{
    std::string moves;
    TreeNode *node = m_engine->m_root;
    while (node->expand_state == k_expanded && node->ch_len > rank) {
        TreeNode *ch = node->ch;
        std::vector<int> idx(node->ch_len);
        std::iota(idx.begin(), idx.end(), 0);
        std::nth_element(idx.begin(), idx.begin() + rank, idx.end(),
                         [ch](int i, int j) { return ch[i].visit_count > ch[j].visit_count; });
        TreeNode *best_ch = &ch[idx[rank]];
        if (moves.size()) moves += ",";
        moves += GoFunction::IdToStr(best_ch->move);
        char buf[100];
        snprintf(buf, sizeof(buf), "(%d,%.2f,%.2f,%.2f)",
                 best_ch->visit_count.load(),
                 (float)best_ch->total_action / k_action_value_base / best_ch->visit_count,
                 best_ch->prior_prob.load(),
                 best_ch->value.load());
        moves += buf;
        node = best_ch;
        rank = 0;
    }
    return moves;
}

void MCTSDebugger::PrintTree(int depth, int topk, const std::string &prefix)
{
    TreeNode *root = m_engine->m_root;
    std::queue<std::pair<TreeNode*, int>> que;
    que.emplace(root, 1);
    while (!que.empty()) {
        TreeNode *node; int dep;
        std::tie(node, dep) = que.front();
        que.pop();

        TreeNode *ch = node->ch;
        std::vector<int> idx(node->ch_len);
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [ch](int i, int j) { return ch[i].visit_count > ch[j].visit_count; });
        if (topk < (int)idx.size()) idx.erase(idx.begin() + topk, idx.end());

        for (int i: idx) {
            if (ch[i].visit_count == 0) {
                break;
            }
            std::string moves;
            for (TreeNode *t = &ch[i]; t != root; t = t->fa) {
                if (moves.size()) moves = "," + moves;
                moves = GoFunction::IdToStr(t->move) + moves;
            }
            VLOG(1) << prefix << moves
                    << ": N=" << ch[i].visit_count
                    << ", W=" << (float)ch[i].total_action / k_action_value_base
                    << ", Q=" << (float)ch[i].total_action / k_action_value_base / ch[i].visit_count
                    << ", p=" << ch[i].prior_prob
                    << ", v=" << ch[i].value;
            if (dep < depth) que.emplace(&ch[i], dep + 1);
        }
    }
}

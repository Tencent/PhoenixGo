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
#include "mcts_engine.h"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>

#include <glog/logging.h>

#include "common/str_utils.h"
#include "model/zero_model.h"
#include "model/trt_zero_model.h"
#include "dist/dist_zero_model_client.h"
#include "dist/async_dist_zero_model_client.h"

static thread_local std::random_device g_random_device;
static thread_local std::minstd_rand g_random_engine(g_random_device());

MCTSEngine::MCTSEngine(const MCTSConfig &config)
    : m_config(config),
      m_root(nullptr),
      m_board(!config.disable_positional_superko()),
      m_eval_task_queue(config.eval_task_queue_size()),
      m_model_global_step(0),
      m_is_searching(false),
      m_simulation_counter(0),
      m_num_moves(0),
      m_gen_passes(0),
      m_monitor(this),
      m_debugger(this)
{
    // setup eval threads
    if (m_config.model_config().enable_mkl()) {
        ZeroModel::SetMKLEnv(m_config.model_config());
    }
    std::vector<int> gpu_list;
    for (const std::string &gpu: SplitStr(m_config.gpu_list(), ',')) {
        gpu_list.push_back(gpu.empty() ? 0 : std::stoi(gpu));
    }
    for (int i = 0; i < m_config.num_eval_threads(); ++i) {
        std::unique_ptr<ZeroModelBase> model;
        if (m_config.enable_dist()) {
            const auto &addr = m_config.dist_svr_addrs(i % m_config.dist_svr_addrs_size());
            if (m_config.enable_async()) {
                model.reset(new AsyncDistZeroModelClient(SplitStr(addr, ','), m_config.dist_config()));
            } else {
                model.reset(new DistZeroModelClient(addr, m_config.dist_config()));
            }
        } else if (m_config.model_config().enable_tensorrt()) {
            model.reset(new TrtZeroModel(gpu_list[i % gpu_list.size()]));
        } else {
            model.reset(new ZeroModel(gpu_list[i % gpu_list.size()]));
        }
        m_eval_threads_init_wg.Add();
        m_eval_threads.emplace_back(&MCTSEngine::EvalRoutine, this, std::move(model));
    }

    // setup search threads
    for (int i = 0; i < m_config.num_search_threads(); ++i) {
        m_search_threads.emplace_back(&MCTSEngine::SearchRoutine, this);
    }

    // setup delete thread & tree root
    m_delete_thread = std::thread(&MCTSEngine::DeleteRoutine, this);
    ChangeRoot(nullptr);

    // wait
    LOG(INFO) << "MCTSEngine: waiting all eval threads init";
    m_eval_threads_init_wg.Wait();
    LOG(INFO) << "MCTSEngine: all eval threads init done";

    if (m_config.enable_background_search()) {
        SearchResume();
    }
}

MCTSEngine::~MCTSEngine()
{
    LOG(INFO) << "~MCTSEngine: Deconstructing MCTSEngine";
    m_search_threads_conductor.Terminate();
    LOG(INFO) << "~MCTSEngine: Waiting search threads terminate";
    for (auto &th: m_search_threads) {
        th.join();
    }
    LOG(INFO) << "~MCTSEngine: Waiting eval threads terminate";
    m_eval_task_queue.Close();
    for (auto &th: m_eval_threads) {
        th.join();
    }
    LOG(INFO) << "~MCTSEngine: Waiting delete thread terminate";
    m_delete_queue.Push(m_root);
    m_delete_queue.Close();
    m_delete_thread.join();
    LOG(INFO) << "~MCTSEngine: Deconstruct MCTSEngin succ";
}

void MCTSEngine::Reset(const std::string &init_moves)
{
    SearchPause();
    ChangeRoot(nullptr);
    m_board.CopyFrom(GoState(!m_config.disable_positional_superko()));
    m_simulation_counter = 0;
    m_num_moves = (init_moves.size() + 1) / 3;
    m_moves_str = init_moves;
    m_gen_passes = 0;
    m_byo_yomi_timer.Reset();

    for (size_t i = 0; i < init_moves.size(); i += 3) {
        GoCoordId x, y;
        GoFunction::StrToCoord(init_moves.substr(i, 2), x, y);
        m_board.Move(x, y);
        m_root->move = GoFunction::CoordToId(x, y);
    }

    if (m_config.enable_background_search()) {
        SearchResume();
    }
}

void MCTSEngine::Move(GoCoordId x, GoCoordId y)
{
    if (!m_byo_yomi_timer.IsEnable()) {
        auto &c = m_config.time_control();
        if (c.main_time() > 0 || c.byo_yomi_time() > 0) {
            m_byo_yomi_timer.Set(c.main_time(), c.byo_yomi_time());
        }
    }

    SearchPause();

    if (GoFunction::IsResign(x, y)) {
        LOG(INFO) << "Move: resign";
        return;
    }

    int ret = m_board.Move(x, y);
    CHECK_EQ(ret, 0) << "Move: failed, " << GoFunction::CoordToStr(x, y) << ", ret" << ret;

    ++m_num_moves;
    if (m_moves_str.size()) m_moves_str += ",";
    m_moves_str += GoFunction::CoordToStr(x, y);
    LOG(INFO) << "Move: " << m_moves_str;

    ChangeRoot(FindChild(m_root, GoFunction::CoordToId(x, y)));
    m_root->move = GoFunction::CoordToId(x, y);

    m_debugger.UpdateLastMoveDebugStr();
    LOG(INFO) << m_debugger.GetLastMoveDebugStr();
    m_debugger.PrintTree(1, 10, GoFunction::CoordToStr(x, y) + ",");

    m_byo_yomi_timer.HandOff();

    if (m_pending_config) {
        m_config = *m_pending_config;
        m_pending_config = nullptr;
        LOG(INFO) << "reload config succ: \n" << m_config.DebugString();
    }

    if (!m_config.disable_double_pass_scoring() && m_board.IsDoublePass()) {
        LOG(INFO) << "Move: double pass, game ends";
        return;
    }

    if (m_config.enable_background_search()) {
        SearchResume();
    } else {
        m_simulation_counter = 0;
    }
}

void MCTSEngine::GenMove(GoCoordId &x, GoCoordId &y)
{
    std::vector<int> visit_count;
    float v_resign;
    GenMove(x, y, visit_count, v_resign);
}

void MCTSEngine::GenMove(GoCoordId &x, GoCoordId &y, std::vector<int> &visit_count, float &v_resign)
{
    if (!m_byo_yomi_timer.IsEnable()) {
        auto &c = m_config.time_control();
        if (c.main_time() > 0 || c.byo_yomi_time() > 0) {
            m_byo_yomi_timer.Set(c.main_time(), c.byo_yomi_time());
        }
    }

    if (m_config.enable_pass_pass() && m_board.GetLastMove() == GoComm::COORD_PASS && !IsPassDisable()) {
        x = y = GoComm::COORD_PASS;
        ++m_gen_passes;
        visit_count = GetVisitCount(m_root);
        v_resign = 1.0f;
        return;
    }

    Search();
    visit_count = GetVisitCount(m_root);

    int move = GoComm::COORD_UNSET;
    if (m_config.genmove_temperature() == 0.0f) {
        move = GetBestMove(v_resign);
    } else {
        move = GetSamplingMove(m_config.genmove_temperature());
        v_resign = 1.0f;
    }
    GoFunction::IdToCoord(move, x, y);

    if (move == GoComm::COORD_PASS) {
        ++m_gen_passes;
    }
}

bool MCTSEngine::Undo()
{
    if (m_num_moves == 0) {
        return false;
    }
    Reset(m_moves_str.substr(0, m_moves_str.size() - 3));
    return true;
}

const GoState &MCTSEngine::GetBoard()
{
    return m_board;
}

MCTSConfig &MCTSEngine::GetConfig()
{
    return m_config;
}

void MCTSEngine::SetPendingConfig(std::unique_ptr<MCTSConfig> config)
{
    m_pending_config = std::move(config);
}

MCTSDebugger &MCTSEngine::GetDebugger()
{
    return m_debugger;
}

int MCTSEngine::GetModelGlobalStep()
{
    return m_model_global_step;
}

ByoYomiTimer &MCTSEngine::GetByoYomiTimer()
{
    return m_byo_yomi_timer;
}

TreeNode *MCTSEngine::InitNode(TreeNode *node, TreeNode *fa, int move, float prior_prob)
{
    node->fa = fa;
    node->ch = nullptr;
    node->ch_len = 0;
    node->size = 1;
    node->expand_state = k_unexpanded;
    node->move = move;
    node->visit_count = 0;
    node->virtual_loss_count = 0;
    node->total_action = 0;
    node->prior_prob = prior_prob;
    node->value = NAN;
    return node;
}

TreeNode *MCTSEngine::FindChild(TreeNode *node, int move)
{
    TreeNode *ch = node->ch;
    int ch_len = node->ch_len;
    for (int i = 0; i < ch_len; ++i) {
        if (ch[i].move == move) {
            return &ch[i];
        }
    }
    return nullptr;
}

void MCTSEngine::Eval(const GoState &board, EvalCallback callback)
{
    if (!m_config.disable_double_pass_scoring() && board.IsDoublePass()) {
        std::vector<float> policy;
        policy.assign(GoComm::GOBOARD_SIZE + 1, 0.0f);
        policy.back() = 1.0f; // pass
        float value = (board.GetWinner() == board.CurrentPlayer()) ? -1.0f : 1.0f;
        callback(0, std::move(policy), value);
        return;
    }

    Timer timer;
    auto features = board.GetFeature();
    int transform_mode = g_random_engine() & 7;
    TransformFeatures(features, transform_mode);

    bool dumb_pass = board.GetWinner() != board.CurrentPlayer();

    callback =
        [this, callback, timer, transform_mode, dumb_pass]
        (int ret, std::vector<float> policy, float value) {
            if (ret == 0) {
                if (dumb_pass && value < 0.5 && !m_config.disable_double_pass_scoring()) {
                    policy.back() = std::min(policy.back(), 1e-5f); // disallow dumb PASS
                }

                CHECK_EQ(policy.size(), GoComm::GOBOARD_SIZE + 1)
                    << "Eval: invalid policy.size(), expect " << GoComm::GOBOARD_SIZE + 1 << ", got " << policy.size();
                if (m_config.enable_policy_temperature()) {
                    ApplyTemperature(policy, m_config.policy_temperature());
                }
                float pass_policy = policy.back();
                policy.pop_back(); // make it 19x19
                TransformFeatures(policy, transform_mode, true);
                policy.push_back(pass_policy);
            }
            m_monitor.MonEvalCostMs(timer.fms());
            callback(ret, std::move(policy), value);
        };

    if (m_config.enable_async()) {
        m_eval_tasks_wg.Add();
        m_eval_task_queue.Push(
            EvalTask {
                features,
                [this, callback](int ret, std::vector<float> policy, float value) {
                    callback(ret, std::move(policy), value);
                    m_eval_tasks_wg.Done();
                }
            }
        );
    } else {
        std::promise<std::tuple<int, std::vector<float>, float>> promise;
        m_eval_task_queue.Push(
            EvalTask {
                features,
                [&promise](int ret, std::vector<float> policy, float value) {
                    promise.set_value(std::make_tuple(ret, std::move(policy), value));
                }
            }
        );
        int ret;
        std::vector<float> policy;
        float value;
        std::tie(ret, policy, value) = promise.get_future().get();
        callback(ret, std::move(policy), value);
    }
    m_monitor.MonTaskQueueSize(m_eval_task_queue.Size());
}

void MCTSEngine::EvalRoutine(std::unique_ptr<ZeroModelBase> model)
{
    int ret = model->Init(m_config.model_config());
    CHECK_EQ(ret, 0) << "EvalRoutine: model init failed, ret " << ret;

    int global_step;
    ret = model->GetGlobalStep(global_step);
    CHECK_EQ(ret, 0) << "EvalRoutine: model get global_step failed, ret " << ret;

    LOG(INFO) << "EvalRoutine: init model done, global_step=" << global_step;
    int expect_zero = 0;
    if (!m_model_global_step.compare_exchange_strong(expect_zero, global_step)) {
        CHECK_EQ(expect_zero, global_step) << "EvalRoutine: global_step different with other routines";
    }

    m_eval_threads_init_wg.Done();
    for (;;) {
        model->Wait();

        EvalTask task;
        std::vector<std::vector<bool>> inputs;
        std::vector<EvalCallback> callbacks;
        for (int i = 0; i < m_config.eval_batch_size(); ++i) {
            if (m_eval_task_queue.Pop(task, i ? m_config.eval_wait_batch_timeout_us() : -1)) {
                inputs.push_back(std::move(task.features));
                callbacks.push_back(std::move(task.callback));
            } else if (m_eval_task_queue.IsClose()) {
                LOG(WARNING) << "EvalRoutine: terminate";
                return; // terminate
            } else { // timeout
                break;
            }
        }

        size_t batch_size = inputs.size();
        m_monitor.MonEvalBatchSize(batch_size);

        Timer timer;
        model->Forward(
            inputs,
            [this, inputs, callbacks, batch_size, timer]
            (int ret, std::vector<std::vector<float>> policy, std::vector<float> value) {
                m_monitor.MonEvalCostMsPerBatch(timer.fms());

                // fill result
                if (ret == ERR_FORWARD_TIMEOUT) {
                    m_monitor.IncEvalTimeout();
                    for (size_t i = 0; i < batch_size; ++i) {
                        m_eval_task_queue.PushFront(EvalTask{std::move(inputs[i]), std::move(callbacks[i])});
                    }
                } else if (ret) {
                    LOG(ERROR) << "EvalRoutine: feed model failed, ret " << ret;
                    for (size_t i = 0; i < batch_size; ++i) {
                        callbacks[i](ret, {}, 0.0);
                    }
                } else {
                    CHECK_EQ(batch_size, policy.size())
                        << "EvalRoutine: batch size unmatch, expect " << batch_size << ", got" << policy.size();
                    CHECK_EQ(batch_size, value.size())
                        << "EvalRoutine: batch size unmatch, expect " << batch_size << ", got" << policy.size();
                    for (size_t i = 0; i < batch_size; ++i) {
                        callbacks[i](ret, std::move(policy[i]), value[i]);
                    }
                }
            }
        );

        if (m_config.enable_async()) {
            m_monitor.MonRpcQueueSize(model->RpcQueueSize());
        }
    }
}

TreeNode *MCTSEngine::Select(GoState &board)
{
    TreeNode *node = m_root;
    int depth = 1;
    ++node->virtual_loss_count;
    while (node->expand_state == k_expanded) {
        node = SelectChild(node);
        ++node->virtual_loss_count;
        int ret = board.Move(node->move);

        CHECK_EQ(ret, 0) << "Move: failed, move=" << m_root->move << ", ret" << ret;
        ++depth;
    }
    m_monitor.MonSearchTreeHeight(depth);
    return node;
}

TreeNode *MCTSEngine::SelectChild(TreeNode *node)
{
    TreeNode *ch = node->ch;
    int ch_len = node->ch_len;
    CHECK_GT(ch_len, 0);
    int visit_count[GoComm::GOBOARD_SIZE + 1];
    float virtual_loss[GoComm::GOBOARD_SIZE + 1];
    float total_action[GoComm::GOBOARD_SIZE + 1];
    for (int i = 0; i < ch_len; ++i) {
        visit_count[i] = ch[i].visit_count;
        virtual_loss[i] = ch[i].virtual_loss_count * m_config.virtual_loss();
        total_action[i] = (float)ch[i].total_action / k_action_value_base;
    }
    float sigma_visit_count = std::accumulate(visit_count, visit_count + ch_len, 0);
    if ((m_config.virtual_loss_mode() & 1) == 0) {
        sigma_visit_count += std::accumulate(virtual_loss, virtual_loss + ch_len, 0.0f);
    }
    float sqrt_sigma_visit_count = std::sqrt(sigma_visit_count);
    sqrt_sigma_visit_count = std::max(sqrt_sigma_visit_count, 1.0f);
    float best;
    TreeNode *best_ch = nullptr;
    float default_act = m_config.default_act();
    if (m_config.inherit_default_act() && node->visit_count) {
        default_act = -(float)node->total_action / k_action_value_base / node->visit_count;
        if (m_config.inherit_default_act_factor() > 0) {
            default_act *= m_config.inherit_default_act_factor();
        }
    }
    for (int i = 0; i < ch_len; ++i) {
        float act;
        if (visit_count[i] == 0 && virtual_loss[i] == 0) {
            act = default_act;
        } else if ((m_config.virtual_loss_mode() & 2) == 0) {
            act = (total_action[i] - virtual_loss[i]) / (visit_count[i] + virtual_loss[i]);
        } else {
            act = total_action[i] / visit_count[i];
        }

        float ucb;
        if ((m_config.virtual_loss_mode() & 1) == 0) {
            ucb = m_config.c_puct() * ch[i].prior_prob *
                  sqrt_sigma_visit_count / (1 + visit_count[i] + virtual_loss[i]);
        } else {
            ucb = m_config.c_puct() * ch[i].prior_prob *
                  sqrt_sigma_visit_count / (1 + visit_count[i]);
        }

        float score = act + ucb;
        if (best_ch == nullptr || score > best) {
            best = score;
            best_ch = &ch[i];
        }
    }
    return best_ch;
}

int MCTSEngine::Expand(TreeNode *node, GoState &board, const std::vector<float> &policy)
{
    if (!m_config.disable_double_pass_scoring() && board.IsDoublePass()) {
        node->expand_state = k_unexpanded;
        return 0;
    }

    Timer timer;
    int ch_len = 0;
    float policy_sum = 0;
    int moves[GoComm::GOBOARD_SIZE + 1];
    for (int i = 0; i < GoComm::GOBOARD_SIZE; ++i) {
        if (board.IsLegal(i)) {
            moves[ch_len++] = i;
            policy_sum += policy[i];
        }
    }
    moves[ch_len++] = GoComm::GOBOARD_SIZE; // PASS
    policy_sum += policy.back(); // PASS

    int max_ch_len = m_config.max_children_per_node();
    if (max_ch_len > 0 && ch_len > max_ch_len) {
        std::nth_element(moves, moves + max_ch_len, moves + ch_len,
                         [&policy](int i, int j) { return policy[i] > policy[j]; });
        ch_len = max_ch_len;
    }

    TreeNode *ch = new TreeNode[ch_len];
    for (int i = 0; i < ch_len; ++i) {
        if (moves[i] == GoComm::GOBOARD_SIZE) {
            InitNode(&ch[i], node, GoComm::COORD_PASS, policy[moves[i]] / policy_sum);
        } else {
            InitNode(&ch[i], node, moves[i], policy[moves[i]] / policy_sum);
        }
    }

    node->ch = ch;
    node->ch_len = ch_len;
    node->expand_state = k_expanded;

    m_monitor.MonExpandCostMs(timer.fms());
    return ch_len;
}

void MCTSEngine::Backup(TreeNode *node, float value_f, int ch_len)
{
    Timer timer;
    node->value = value_f;
    int64_t value = value_f * k_action_value_base;
    int depth = 1;
    while (node != nullptr) {
        node->size += ch_len;
        ++depth;
        ++node->visit_count;
        --node->virtual_loss_count;
        node->total_action += value;
        node = node->fa;
        value = -value;
        value_f = -value_f;
    }
    m_monitor.MonBackupCostMs(timer.fms());
}

void MCTSEngine::UndoVirtualLoss(TreeNode *node)
{
    while (node != nullptr) {
        --node->virtual_loss_count;
        node = node->fa;
    }
}

bool MCTSEngine::CheckEarlyStop(int64_t timeout_us)
{
    auto &c = m_config.early_stop();
    if (!c.enable() || m_simulation_counter < c.sims_threshold()) {
        return false;
    }
    int max_visit_count[] = {0, 0};
    TreeNode *ch = m_root->ch;
    int ch_len = m_root->ch_len;
    for (int i = 0; i < ch_len; ++i) {
        int visit_count = ch[i].visit_count;
        if (visit_count > max_visit_count[1]) {
            if (visit_count > max_visit_count[0]) {
                max_visit_count[1] = max_visit_count[0];
                max_visit_count[0] = visit_count;
            } else {
                max_visit_count[1] = visit_count;
            }
        }
    }
    int64_t elapsed_us = m_search_timer.us();
    float remain_sims = (float)m_simulation_counter * (timeout_us - elapsed_us) / elapsed_us;
    if (c.sims_factor() > 0) {
        remain_sims *= c.sims_factor();
    }
    if (max_visit_count[0] - max_visit_count[1] > remain_sims) {
        LOG(INFO) << "CheckEarlyStop: return true"
                  << ", max N=" << max_visit_count[0] << ", second N=" << max_visit_count[1]
                  << ", diff=" << max_visit_count[0] - max_visit_count[1]
                  << ", elapsed " << elapsed_us / 1000.0 << "ms, remain_sims=" << remain_sims;
        return true;
    }
    return false;
}

bool MCTSEngine::CheckUnstable()
{
    auto &c = m_config.unstable_overtime();
    if (!c.enable()) {
        return false;
    }
    TreeNode *ch = m_root->ch;
    int ch_len = m_root->ch_len;
    int visit_count[GoComm::GOBOARD_SIZE + 1];
    float mean_action[GoComm::GOBOARD_SIZE + 1];
    for (int i = 0; i < ch_len; ++i) {
        visit_count[i] = ch[i].visit_count;
        mean_action[i] = visit_count[i] == 0 ? 0.0f :
                         (float)ch[i].total_action / k_action_value_base / visit_count[i];
    }
    int n_best = std::max_element(visit_count, visit_count + ch_len) - visit_count;
    int q_best = std::max_element(mean_action, mean_action + ch_len) - mean_action;
    if (n_best != q_best) {
        LOG(INFO) << "CheckUnstable: return true"
                  << ", N best ch=" << GoFunction::IdToStr(ch[n_best].move)
                  << ", N=" << visit_count[n_best] << ", Q=" << mean_action[n_best]
                  << ", Q best ch=" << GoFunction::IdToStr(ch[q_best].move)
                  << ", N=" << visit_count[q_best] << ", Q=" << mean_action[q_best];
        return true;
    }
    return false;
}

bool MCTSEngine::CheckBehind()
{
    auto &c = m_config.behind_overtime();
    if (!c.enable()) {
        return false;
    }
    float v_resign;
    int best_move = GetBestMove(v_resign);
    if (best_move == GoComm::COORD_RESIGN) {
        LOG(INFO) << "CheckBehind: return true cause best_move=resign";
        return true;
    }
    TreeNode *best_ch = FindChild(m_root, best_move);
    int visit_count = best_ch->visit_count;
    float mean_action = visit_count == 0 ? 0.0f :
                        (float)best_ch->total_action / k_action_value_base / visit_count;
    if (mean_action < c.act_threshold()) {
        LOG(INFO) << "CheckBehind: return true, best_move=" << GoFunction::IdToStr(best_move)
                  << ", N=" << visit_count << ", Q=" << mean_action;
        return true;
    }
    return false;
}

int64_t MCTSEngine::GetSearchTimeoutUs()
{
    int64_t timeout_us = -1;
    auto &c = m_config.time_control();
    if (m_byo_yomi_timer.IsEnable() && c.enable()) {
        float overtime_factor = 1.0f;
        if (m_config.unstable_overtime().enable()) {
            overtime_factor = std::max(overtime_factor, 1.0f + m_config.unstable_overtime().time_factor());
        }
        if (m_config.behind_overtime().enable()) {
            overtime_factor = std::max(overtime_factor, 1.0f + m_config.behind_overtime().time_factor());
        }
        float remain_time = m_byo_yomi_timer.GetRemainTime(m_board.CurrentPlayer());
        float byo_yomi_time = m_byo_yomi_timer.GetByoYomiTime();
        float think_time;
        if (remain_time > 0) {
            think_time = remain_time / (c.c_denom() + std::max(c.c_maxply() - m_num_moves, 0));
            think_time = std::min(think_time, (remain_time - c.reserved_time()) / overtime_factor);
            if (m_num_moves >= c.byo_yomi_after()) {
                think_time = std::max(think_time, (byo_yomi_time - c.reserved_time()) / overtime_factor);
            }
        } else {
            think_time = (byo_yomi_time - c.reserved_time()) / overtime_factor;
        }
        think_time = std::max(think_time, c.min_time());
        timeout_us = think_time * 1e6;
    }
    if (m_config.timeout_ms_per_step() > 0) {
        int64_t t = m_config.timeout_ms_per_step() * 1000LL;
        if (timeout_us == -1 || t < timeout_us) {
            timeout_us = t;
        }
    }
    if (timeout_us != -1) {
        LOG(INFO) << "GetSearchTimeoutUs: timeout=" << timeout_us / 1e6 << "s";
    }
    return timeout_us;
}

int64_t MCTSEngine::GetSearchOvertimeUs(int64_t timeout_us)
{
    if (timeout_us > 0) {
        if (CheckUnstable()) {
            return timeout_us * m_config.unstable_overtime().time_factor();
        }
        if (CheckBehind()) {
            return timeout_us * m_config.behind_overtime().time_factor();
        }
    }
    return 0;
}

void MCTSEngine::Search()
{
    SearchResume();

    int64_t timeout_us = GetSearchTimeoutUs();
    SearchWait(timeout_us, false);

    int64_t overtime_us = GetSearchOvertimeUs(timeout_us);
    SearchWait(overtime_us, true);

    SearchPause();
}

void MCTSEngine::SearchWait(int64_t timeout_us, bool is_overtime)
{
    if (timeout_us == 0) {
        return;
    }
    if (timeout_us < 0) {
        m_search_threads_conductor.Join();
        return;
    }
    if (!m_config.early_stop().enable()) {
        m_search_threads_conductor.Join(timeout_us);
        return;
    }

    timeout_us += m_search_timer.us();
    int64_t check_every_us = m_config.early_stop().check_every_ms() * 1000LL;
    while (m_search_timer.us() + check_every_us < timeout_us) {
        if (m_search_threads_conductor.Join(check_every_us)) {
            return; // all search threads has pause
        }
        if (CheckEarlyStop(timeout_us)) {
            if (!is_overtime) {
                int64_t overtime_us = GetSearchOvertimeUs(timeout_us);
                if (overtime_us > 0 && !CheckEarlyStop(timeout_us + overtime_us)) {
                    break; // don't early stop
                }
            }
            return;
        }
    }
    int64_t remain_us = timeout_us - m_search_timer.us();
    if (remain_us > 0) {
        m_search_threads_conductor.Join(remain_us);
    }
}

void MCTSEngine::SearchResume()
{
    if (!m_is_searching) {
        m_simulation_counter = 0;
        m_search_timer.Reset();

        if (m_config.clear_search_tree_per_move()) {
            ChangeRoot(nullptr);
        }

        InitRoot();

        m_monitor.Reset();
        m_monitor.Resume();
        m_search_threads_conductor.Resume(m_search_threads.size());

        m_is_searching = true;
    }
}

void MCTSEngine::SearchPause()
{
    if (m_is_searching) {
        m_search_threads_conductor.Pause();
        m_search_threads_conductor.Join();
        m_eval_tasks_wg.Wait();
        m_monitor.Pause();
        m_monitor.Log();
        m_debugger.Debug();

        m_is_searching = false;
    }
}

void MCTSEngine::SearchRoutine()
{
    m_search_threads_conductor.Wait();
    if (m_search_threads_conductor.IsTerminate()) {
        LOG(WARNING) << "SearchRoutine: terminate";
        return;
    }
    for (;;) {
        if (!m_search_threads_conductor.IsRunning()) {
            VLOG(2) << "SearchRoutine pause";
            m_search_threads_conductor.AckPause();
            m_search_threads_conductor.Wait();
            VLOG(2) << "SearchRoutine resume";
            if (m_search_threads_conductor.IsTerminate()) {
                LOG(WARNING) << "SearchRoutine: terminate";
                return;
            }
        }
        Timer timer;
        auto board = std::make_shared<GoState>(m_board);
        TreeNode *node = Select(*board);
        m_monitor.MonSelectCostMs(timer.fms());

        int expect_unexpanded = k_unexpanded;
        if (node->expand_state.compare_exchange_strong(expect_unexpanded, k_expanding)) {
            Eval(*board, [this, node, board, timer](int ret, std::vector<float> policy, float value) {
                if (ret) {
                    node->expand_state = k_unexpanded;
                    UndoVirtualLoss(node);
                } else {
                    int ch_len = Expand(node, *board, policy);
                    Backup(node, value, ch_len);

                    ++m_simulation_counter;
                    if (m_config.max_simulations_per_step() > 0 &&
                        m_simulation_counter > m_config.max_simulations_per_step()) {
                        m_search_threads_conductor.Pause();
                    }

                    if (m_root->size > m_config.max_search_tree_size() &&
                        m_search_threads_conductor.IsRunning()) {
                        m_search_threads_conductor.Pause();
                        LOG(ERROR) << "Expand: node pool exhausted, search pause";
                    }
                }
                m_monitor.MonSimulationCostMs(timer.fms());
            });
        } else {
            UndoVirtualLoss(node);
            m_monitor.IncSelectSameNode();
        }
    }
}

void MCTSEngine::ChangeRoot(TreeNode *node)
{
    TreeNode *root = new TreeNode;
    if (node) {
        root->fa = nullptr;
        root->size = node->size.load();
        root->expand_state = node->expand_state.load();
        root->move = node->move.load();
        root->visit_count = node->visit_count.load();
        root->virtual_loss_count = node->virtual_loss_count.load();
        root->total_action = node->total_action.load();
        root->prior_prob = node->prior_prob.load();
        root->value = node->value.load();

        TreeNode *ch = node->ch;
        int ch_len = node->ch_len;
        for (int i = 0; i < ch_len; ++i) {
            ch[i].fa = root;
        }
        root->ch = ch;
        root->ch_len = ch_len;
        node->ch = nullptr;
        node->ch_len = 0;
    } else {
        InitNode(root, nullptr, -1, 0.0);
    }

    if (m_root) {
        m_delete_queue.Push(m_root);
    }
    m_root = root;
}

void MCTSEngine::InitRoot()
{
    CHECK_NOTNULL(m_root);
    while (m_root->expand_state == k_unexpanded) {
        Eval(m_board, [this](int ret, std::vector<float> policy, float value) {
            if (ret) {
                LOG(ERROR) << "InitRoot: eval root node failed, ret " << ret;
            } else {
                int ch_len = Expand(m_root, m_board, policy);
                Backup(m_root, value, ch_len);
            }
        });
        m_eval_tasks_wg.Wait();
    }
    if (m_config.enable_dirichlet_noise()) {
        TreeNode *ch = m_root->ch;
        int ch_len = m_root->ch_len;
        float noise[GoComm::GOBOARD_SIZE + 1];
        std::gamma_distribution<float> gamma(m_config.dirichlet_noise_alpha());
        for (int i = 0; i < ch_len; ++i) {
            noise[i] = gamma(g_random_engine);
        }
        float noise_sum = std::accumulate(noise, noise + ch_len, 0.0f);
        for (int i = 0; i < ch_len; ++i) {
            ch[i].prior_prob = (1 - m_config.dirichlet_noise_ratio()) * ch[i].prior_prob +
                                m_config.dirichlet_noise_ratio() * noise[i] / noise_sum;
        }
        bool dumb_pass = m_board.GetWinner() != m_board.CurrentPlayer();
        if (dumb_pass && m_root->value < 0.5 && !m_config.disable_double_pass_scoring()) {
            for (int i = 0; i < ch_len; ++i) {
                if (ch[i].move == GoComm::COORD_PASS) {
                    ch[i].prior_prob = 1e-5f;
                }
            }
        }
    }
}

void MCTSEngine::DeleteRoutine()
{
    for (;;) {
        TreeNode *node;
        if (m_delete_queue.Pop(node)) {
            Timer timer;
            int size = DeleteTree(node);
            delete node;
            LOG(INFO) << "DeleteRoutine: deleted " << size + 1 << " nodes, cost " << timer.fms() << "ms";
        } else {
            LOG(WARNING) << "DeleteRoutine: terminate";
            return; // terminate
        }
    }
}

int MCTSEngine::DeleteTree(TreeNode *node)
{
    TreeNode *ch = node->ch;
    int ch_len = node->ch_len;
    int size = ch_len;
    for (int i = 0; i < ch_len; ++i) {
        size += DeleteTree(&ch[i]);
    }
    delete[] ch;
    return size;
}

int MCTSEngine::GetBestMove(float &v_resign)
{
    TreeNode *ch = m_root->ch;
    int ch_len = m_root->ch_len;
    int visit_count[GoComm::GOBOARD_SIZE + 1];
    float total_action[GoComm::GOBOARD_SIZE + 1];
    float mean_action[GoComm::GOBOARD_SIZE + 1];
    float prior_prob[GoComm::GOBOARD_SIZE + 1];
    float value[GoComm::GOBOARD_SIZE + 1];
    bool disable_pass = IsPassDisable();
    for (int i = 0; i < ch_len; ++i) {
        if (disable_pass && ch[i].move == GoComm::COORD_PASS) {
            visit_count[i] = 0;
            total_action[i] = 0.0f;
            mean_action[i] = 0.0f;
            prior_prob[i] = 0.0f;
            value[i] = -1.0f;
        } else {
            visit_count[i] = ch[i].visit_count;
            total_action[i] = (float)ch[i].total_action / k_action_value_base;
            mean_action[i] = visit_count[i] == 0 ? -1.0f : total_action[i] / visit_count[i];
            prior_prob[i] = ch[i].prior_prob;
            value[i] = ch[i].value;
        }

        VLOG(2) << "GetBestMove: " << GoFunction::IdToStr(ch[i].move)
                << ", N " << visit_count[i] << ", W " << total_action[i] << ", Q " << mean_action[i]
                << ", p " << prior_prob[i] << ", v " << value[i];
    }
    int choice = 0;
    switch (m_config.get_best_move_mode()) {
        case 0: choice = std::max_element(visit_count,  visit_count  + ch_len) - visit_count;  break;
        case 1: choice = std::max_element(total_action, total_action + ch_len) - total_action; break;
        case 2: choice = std::max_element(mean_action,  mean_action  + ch_len) - mean_action;  break;
        case 3: choice = std::max_element(prior_prob,   prior_prob   + ch_len) - prior_prob;   break;
        case 4: choice = std::max_element(value,        value        + ch_len) - value;        break;
        default: LOG(FATAL) << "GetBestMove: wrong get_best_move_mode " << m_config.get_best_move_mode();
    }

    float root_action = -(float)m_root->total_action / k_action_value_base / m_root->visit_count;
    float root_value = -m_root->value;
    switch (m_config.resign_mode()) {
        case 0: v_resign = std::max(root_action, mean_action[choice]); break;
        case 1: v_resign = std::max(root_value,        value[choice]); break;
        case 2: v_resign = std::max(root_action, *std::max_element(mean_action, mean_action + ch_len)); break;
        case 3: v_resign = std::max(root_value,  *std::max_element(value,       value       + ch_len)); break;
        default: LOG(FATAL) << "GetBestMove: wrong resign_mode " << m_config.resign_mode();
    }

    if (m_config.enable_resign() && v_resign < m_config.v_resign()) {
        LOG(WARNING) << "GetBestMove: resign, v_resign=" << v_resign;
        return GoComm::COORD_RESIGN;
    }
    LOG(INFO) << "GetBestMove: v_resign=" << v_resign << ", not resign";
    return ch[choice].move;
}

int MCTSEngine::GetSamplingMove(float temperature)
{
    TreeNode *ch = m_root->ch;
    int ch_len = m_root->ch_len;
    float rtemp = 1.0f / temperature;
    float probs[GoComm::GOBOARD_SIZE + 1];
    bool disable_pass = IsPassDisable();
    for (int i = 0; i < ch_len; ++i) {
        if (disable_pass && ch[i].move == GoComm::COORD_PASS) {
            probs[i] = 0.0;
        } else {
            probs[i] = std::pow(ch[i].visit_count, rtemp);
        }
    }
    int choice = std::discrete_distribution<>(probs, probs + ch_len)(g_random_engine);
    return ch[choice].move;
}

std::vector<int> MCTSEngine::GetVisitCount(TreeNode *node)
{
    TreeNode *ch = node->ch;
    int ch_len = node->ch_len;
    std::vector<int> visit_count(GoComm::GOBOARD_SIZE + 1, 0);
    for (int i = 0; i < ch_len; ++i) {
        int move = ch[i].move;
        if (move == GoComm::COORD_PASS) {
            visit_count.back() = ch[i].visit_count;
        } else {
            visit_count[move] = ch[i].visit_count;
        }
    }
    return visit_count;
}

template<class T>
void MCTSEngine::TransformFeatures(T &features, int mode, bool reverse)
{
    if (m_config.disable_transform()) {
        return;
    }

    T ret(features.size());
    int depth = features.size() / GoComm::GOBOARD_SIZE;
    for (int i = 0; i < GoComm::GOBOARD_SIZE; ++i) {
        GoCoordId x, y;
        GoFunction::IdToCoord(i, x, y);
        TransformCoord(x, y, mode, reverse);
        int j = GoFunction::CoordToId(x, y);
        for (int k = 0; k < depth; ++k) {
            ret[i * depth + k] = features[j * depth + k];
        }
    }
    features = std::move(ret);
}

void MCTSEngine::TransformCoord(GoCoordId &x, GoCoordId &y, int mode, bool reverse)
{
    if (reverse) {
        if (mode & 4) std::swap(x, y);
        if (mode & 2) y = GoComm::BORDER_SIZE - y - 1;
        if (mode & 1) x = GoComm::BORDER_SIZE - x - 1;
    } else {
        if (mode & 1) x = GoComm::BORDER_SIZE - x - 1;
        if (mode & 2) y = GoComm::BORDER_SIZE - y - 1;
        if (mode & 4) std::swap(x, y);
    }
}

void MCTSEngine::ApplyTemperature(std::vector<float> &probs, float temperature)
{
    float rtemp = 1.0f / temperature;
    for (float &p: probs) {
        p = std::pow(p, rtemp);
    }
    float s = std::accumulate(probs.begin(), probs.end(), 0.0f);
    for (float &p: probs) {
        p /= s;
    }
}

bool MCTSEngine::IsPassDisable()
{
    return m_config.disable_pass() ||
           (m_config.max_gen_passes() && m_gen_passes >= m_config.max_gen_passes());
}

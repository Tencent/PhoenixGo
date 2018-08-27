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

#include <thread>
#include <vector>
#include <algorithm>

#include <glog/logging.h>
#include "common/thread_conductor.h"

class LocalMonitor
{
    friend class MCTSMonitor;

    struct Average
    {
        float avg;
        int cnt;

        Average(float avg = 0, int cnt = 0): avg(avg), cnt(cnt) {}
        void Update(float v) { avg += (v - avg) / ++cnt; }
        void Update(const Average &v) { if (v.cnt) avg += (v.avg - avg) * v.cnt / (cnt += v.cnt); }
    };

    template<class T>
    static void UpdateMax(T &a, T b) { if (a < b) a = b; }

 public:
    LocalMonitor(int id): m_id(id) { Reset(); }

    void Reset()
    {
        m_max_eval_cost_ms = 0;
        m_avg_eval_cost_ms = 0;

        m_max_eval_cost_ms_per_batch = 0;
        m_avg_eval_cost_ms_per_batch = 0;

        m_max_sim_cost_ms = 0;
        m_avg_sim_cost_ms = 0;

        m_max_select_cost_ms = 0;
        m_avg_select_cost_ms = 0;

        m_max_expand_cost_ms = 0;
        m_avg_expand_cost_ms = 0;

        m_max_backup_cost_ms = 0;
        m_avg_backup_cost_ms = 0;

        m_avg_batch_size = 0;

        m_eval_timeout = 0;

        m_select_same_node = 0;

        m_max_tree_height = 0;
        m_avg_tree_height = 0;

        m_avg_task_queue_size = 0;

        m_avg_rpc_queue_size = 0;
    }

    void MonEvalCostMs(float cost_ms)
    {
        UpdateMax(m_max_eval_cost_ms, cost_ms);
        m_avg_eval_cost_ms.Update(cost_ms);
    }

    void MonEvalCostMsPerBatch(float cost_ms)
    {
        UpdateMax(m_max_eval_cost_ms_per_batch, cost_ms);
        m_avg_eval_cost_ms_per_batch.Update(cost_ms);
    }

    void MonSimulationCostMs(float cost_ms)
    {
        UpdateMax(m_max_sim_cost_ms, cost_ms);
        m_avg_sim_cost_ms.Update(cost_ms);
    }

    void MonSelectCostMs(float cost_ms)
    {
        UpdateMax(m_max_select_cost_ms, cost_ms);
        m_avg_select_cost_ms.Update(cost_ms);
    }

    void MonExpandCostMs(float cost_ms)
    {
        UpdateMax(m_max_expand_cost_ms, cost_ms);
        m_avg_expand_cost_ms.Update(cost_ms);
    }

    void MonBackupCostMs(float cost_ms)
    {
        UpdateMax(m_max_backup_cost_ms, cost_ms);
        m_avg_backup_cost_ms.Update(cost_ms);
    }

    void MonEvalBatchSize(int batch_size)
    {
        m_avg_batch_size.Update(batch_size);
    }

    void IncEvalTimeout()
    {
        ++m_eval_timeout;
    }

    void IncSelectSameNode()
    {
        ++m_select_same_node;
    }

    void MonSearchTreeHeight(int height)
    {
        UpdateMax(m_max_tree_height, height);
        m_avg_tree_height.Update(height);
    }

    void MonTaskQueueSize(int size)
    {
        m_avg_task_queue_size.Update(size);
    }

    void MonRpcQueueSize(int size)
    {
        m_avg_rpc_queue_size.Update(size);
    }

 private:
    int m_id;

    float m_max_eval_cost_ms;
    Average m_avg_eval_cost_ms;

    float m_max_eval_cost_ms_per_batch;
    Average m_avg_eval_cost_ms_per_batch;

    float m_max_sim_cost_ms;
    Average m_avg_sim_cost_ms;

    float m_max_select_cost_ms;
    Average m_avg_select_cost_ms;

    float m_max_expand_cost_ms;
    Average m_avg_expand_cost_ms;

    float m_max_backup_cost_ms;
    Average m_avg_backup_cost_ms;

    Average m_avg_batch_size;

    int m_eval_timeout;

    int m_select_same_node;

    int m_max_tree_height;
    Average m_avg_tree_height;

    Average m_avg_task_queue_size;

    Average m_avg_rpc_queue_size;
};

class MCTSEngine;
class MCTSMonitor
{
 public:
    MCTSMonitor(MCTSEngine *engine);
    ~MCTSMonitor();

    void Pause();
    void Resume();
    void Reset();
    void Log();
    void MonitorRoutine();

 private:
    LocalMonitor &GetLocal()
    {
        if (g_local_monitors[m_slot] == nullptr || g_local_monitors[m_slot]->m_id != m_id) {
            g_local_monitors[m_slot] = std::make_shared<LocalMonitor>(m_id);
            std::lock_guard<std::mutex> lock(m_local_monitors_mutex);
            m_local_monitors.push_back(g_local_monitors[m_slot]);
        }
        return *g_local_monitors[m_slot];
    }

    template<class T, class Fn>
    T GetGlobal(T LocalMonitor::*field, Fn update_fn)
    {
        T ret = 0;
        std::lock_guard<std::mutex> lock(m_local_monitors_mutex);
        for (auto &local_monitor: m_local_monitors) {
            update_fn(ret, local_monitor.get()->*field);
        }
        return ret;
    }

    template<class T>
    T GetGlobalSum(T LocalMonitor::*field)
    {
        return GetGlobal(field, [](T &a, const T &b) { a += b; });
    }

    template<class T>
    T GetGlobalMax(T LocalMonitor::*field)
    {
        return GetGlobal(field, [](T &a, const T &b) { if (a < b) a = b; });
    }

    template<class T>
    float GetGlobalAvg(T LocalMonitor::*field)
    {
        return GetGlobal(field, [](T &a, const T &b) { a.Update(b); }).avg;
    }

 public:
    void MonEvalCostMs(float cost_ms)         { GetLocal().MonEvalCostMs(cost_ms); }
    void MonEvalCostMsPerBatch(float cost_ms) { GetLocal().MonEvalCostMsPerBatch(cost_ms); }
    void MonSimulationCostMs(float cost_ms)   { GetLocal().MonSimulationCostMs(cost_ms); }
    void MonSelectCostMs(float cost_ms)       { GetLocal().MonSelectCostMs(cost_ms); }
    void MonExpandCostMs(float cost_ms)       { GetLocal().MonExpandCostMs(cost_ms); }
    void MonBackupCostMs(float cost_ms)       { GetLocal().MonBackupCostMs(cost_ms); }
    void MonEvalBatchSize(int batch_size)     { GetLocal().MonEvalBatchSize(batch_size); }
    void IncEvalTimeout()                     { GetLocal().IncEvalTimeout(); }
    void IncSelectSameNode()                  { GetLocal().IncSelectSameNode(); }
    void MonSearchTreeHeight(int height)      { GetLocal().MonSearchTreeHeight(height); }
    void MonTaskQueueSize(int size)           { GetLocal().MonTaskQueueSize(size); }
    void MonRpcQueueSize(int size)            { GetLocal().MonRpcQueueSize(size); }

    float MaxEvalCostMs()         { return GetGlobalMax(&LocalMonitor::m_max_eval_cost_ms); }
    float AvgEvalCostMs()         { return GetGlobalAvg(&LocalMonitor::m_avg_eval_cost_ms); }
    float MaxEvalCostMsPerBatch() { return GetGlobalMax(&LocalMonitor::m_max_eval_cost_ms_per_batch); }
    float AvgEvalCostMsPerBatch() { return GetGlobalAvg(&LocalMonitor::m_avg_eval_cost_ms_per_batch); }
    float MaxSimulationCostMs()   { return GetGlobalMax(&LocalMonitor::m_max_sim_cost_ms); }
    float AvgSimulationCostMs()   { return GetGlobalAvg(&LocalMonitor::m_avg_sim_cost_ms); }
    float MaxSelectCostMs()       { return GetGlobalMax(&LocalMonitor::m_max_select_cost_ms); }
    float AvgSelectCostMs()       { return GetGlobalAvg(&LocalMonitor::m_avg_select_cost_ms); }
    float MaxExpandCostMs()       { return GetGlobalMax(&LocalMonitor::m_max_expand_cost_ms); }
    float AvgExpandCostMs()       { return GetGlobalAvg(&LocalMonitor::m_avg_expand_cost_ms); }
    float MaxBackupCostMs()       { return GetGlobalMax(&LocalMonitor::m_max_backup_cost_ms); }
    float AvgBackupCostMs()       { return GetGlobalAvg(&LocalMonitor::m_avg_backup_cost_ms); }
    float AvgEvalBatchSize()      { return GetGlobalAvg(&LocalMonitor::m_avg_batch_size); }
    int   EvalTimeout()           { return GetGlobalSum(&LocalMonitor::m_eval_timeout); }
    int   SelectSameNode()        { return GetGlobalSum(&LocalMonitor::m_select_same_node); }
    int   MaxSearchTreeHeight()   { return GetGlobalMax(&LocalMonitor::m_max_tree_height); }
    float AvgSearchTreeHeight()   { return GetGlobalAvg(&LocalMonitor::m_avg_tree_height); }
    float AvgTaskQueueSize()      { return GetGlobalAvg(&LocalMonitor::m_avg_task_queue_size); }
    float AvgRpcQueueSize()       { return GetGlobalAvg(&LocalMonitor::m_avg_rpc_queue_size); }

 private:
    MCTSEngine *m_engine;
    std::thread m_monitor_thread;
    ThreadConductor m_monitor_thread_conductor;

    int m_id;
    int m_slot;
    std::vector<std::shared_ptr<LocalMonitor>> m_local_monitors;
    std::mutex m_local_monitors_mutex;

    static const int k_max_monitor_instances = 1000;
    static int g_next_monitor_id;
    static MCTSMonitor *g_global_monitors[k_max_monitor_instances];
    static std::mutex g_global_monitors_mutex;
    static thread_local std::shared_ptr<LocalMonitor> g_local_monitors[k_max_monitor_instances];
};

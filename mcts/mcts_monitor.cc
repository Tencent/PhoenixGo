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
#include "mcts_monitor.h"

#include <glog/logging.h>

#include "mcts_engine.h"

int MCTSMonitor::g_next_monitor_id = 0;
MCTSMonitor *MCTSMonitor::g_global_monitors[k_max_monitor_instances];
std::mutex MCTSMonitor::g_global_monitors_mutex;
thread_local std::shared_ptr<LocalMonitor> MCTSMonitor::g_local_monitors[k_max_monitor_instances];

MCTSMonitor::MCTSMonitor(MCTSEngine *engine)
    : m_engine(engine), m_slot(0)
{
    {
        std::lock_guard<std::mutex> lock(g_global_monitors_mutex);
        m_id = g_next_monitor_id++;
        while (m_slot < k_max_monitor_instances && g_global_monitors[m_slot]) ++m_slot;
        CHECK(m_slot < k_max_monitor_instances) << "Too many MCTSMonitor instances";
        g_global_monitors[m_slot] = this;
    }
    if (m_engine->GetConfig().monitor_log_every_ms() > 0) {
        m_monitor_thread = std::thread(&MCTSMonitor::MonitorRoutine, this);
    }
}

MCTSMonitor::~MCTSMonitor()
{
    if (m_monitor_thread.joinable()) {
        m_monitor_thread_conductor.Terminate();
        m_monitor_thread.join();
    }
    g_local_monitors[m_slot] = nullptr;
    std::lock_guard<std::mutex> lock(g_global_monitors_mutex);
    g_global_monitors[m_slot] = nullptr;
}

void MCTSMonitor::Pause()
{
    m_monitor_thread_conductor.Pause();
    m_monitor_thread_conductor.Join();
}

void MCTSMonitor::Resume()
{
    if (m_engine->GetConfig().monitor_log_every_ms() > 0) {
        if (!m_monitor_thread.joinable()) {
            m_monitor_thread = std::thread(&MCTSMonitor::MonitorRoutine, this);
        }
        m_monitor_thread_conductor.Resume(1);
    }
}

void MCTSMonitor::Reset()
{
    std::lock_guard<std::mutex> lock(m_local_monitors_mutex);
    for (auto &local_monitor: m_local_monitors) {
        local_monitor->Reset();
    }
    m_local_monitors.erase(
        std::remove_if(m_local_monitors.begin(), m_local_monitors.end(),
                       [](const std::shared_ptr<LocalMonitor> &ptr) { return ptr.use_count() == 1; }),
        m_local_monitors.end());
}

void MCTSMonitor::Log()
{
    VLOG(0) << "MCTSMonitor: avg eval cost " << AvgEvalCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: max eval cost " << MaxEvalCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: avg eval cost " << AvgEvalCostMsPerBatch() << "ms per batch";
    VLOG(0) << "MCTSMonitor: max eval cost " << MaxEvalCostMsPerBatch() << "ms per batch";
    VLOG(0) << "MCTSMonitor: avg eval batch size " << AvgEvalBatchSize();
    VLOG(0) << "MCTSMonitor: eval timeout " << EvalTimeout() << " times";
    VLOG(0) << "MCTSMonitor: avg simulation cost " << AvgSimulationCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: max simulation cost " << MaxSimulationCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: avg select cost " << AvgSelectCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: max select cost " << MaxSelectCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: avg expand cost " << AvgExpandCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: max expand cost " << MaxExpandCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: avg backup cost " << AvgBackupCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: max backup cost " << MaxBackupCostMs() << "ms";
    VLOG(0) << "MCTSMonitor: select same node " << SelectSameNode() << " times";
    VLOG(0) << "MCTSMonitor: search tree height is " << MaxSearchTreeHeight();
    VLOG(0) << "MCTSMonitor: avg height of nodes is " << AvgSearchTreeHeight();
    VLOG(0) << "MCTSMonitor: avg eval task queue size is " << AvgTaskQueueSize();


    if (m_engine->GetConfig().enable_async()) {
        VLOG(0) << "MCTSMonitor: avg rpc queue size is " << AvgRpcQueueSize();
    }
}

void MCTSMonitor::MonitorRoutine()
{
    m_monitor_thread_conductor.Wait();
    for (;;) {
        if (!m_monitor_thread_conductor.IsRunning()) {
            m_monitor_thread_conductor.AckPause();
            m_monitor_thread_conductor.Wait();
            if (m_monitor_thread_conductor.IsTerminate()) {
                LOG(WARNING) << "MonitorRoutine: terminate";
                return;
            }
        }
        m_monitor_thread_conductor.Sleep(m_engine->GetConfig().monitor_log_every_ms() * 1000LL);
        if (m_monitor_thread_conductor.IsRunning()) {
            Log();
        }
        google::FlushLogFiles(google::GLOG_INFO);
    }
}

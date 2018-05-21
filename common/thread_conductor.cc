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
#include "thread_conductor.h"

#include <chrono>

ThreadConductor::ThreadConductor() : m_state(k_pause)
{
}

ThreadConductor::~ThreadConductor()
{
}

void ThreadConductor::Pause()
{
    m_resume_wg.Wait();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = k_pause;
    m_cond.notify_all();
}

void ThreadConductor::Resume(int num_threads)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_state == k_running) return;
        m_resume_wg.Add(num_threads);
        m_pause_wg.Add(num_threads);
        m_state = k_running;
    }
    m_cond.notify_all();
    m_resume_wg.Wait();
}

void ThreadConductor::Wait()
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this]{ return m_state != k_pause; });
        if (m_state == k_terminate) return;
    }
    m_resume_wg.Done();
}

void ThreadConductor::AckPause()
{
    m_pause_wg.Done();
}

bool ThreadConductor::Join(int64_t timeout_us)
{
    return m_pause_wg.Wait(timeout_us);
}

void ThreadConductor::Sleep(int64_t duration_us)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait_for(lock, std::chrono::microseconds(duration_us), [this]{ return m_state == k_pause; });
}

bool ThreadConductor::IsRunning()
{
    return m_state == k_running;
}

void ThreadConductor::Terminate()
{
    Pause();
    Join();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state = k_terminate;
    }
    m_cond.notify_all();
}

bool ThreadConductor::IsTerminate()
{
    return m_state == k_terminate;
}

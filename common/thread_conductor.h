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

#include <atomic>
#include <mutex>
#include <condition_variable>

#include "wait_group.h"


class ThreadConductor
{
 public:
    ThreadConductor();
    ~ThreadConductor();

    void Pause();
    void Resume(int num_threads);
    void Wait();
    void AckPause();
    bool Join(int64_t timeout_us = -1);
    void Sleep(int64_t duration_us);
    bool IsRunning();
    void Terminate();
    bool IsTerminate();

 private:
    std::atomic<int> m_state;
    std::mutex m_mutex;
    std::condition_variable m_cond;

    WaitGroup m_resume_wg;
    WaitGroup m_pause_wg;

 private:
    static const int k_pause     = 0;
    static const int k_running   = 1;
    static const int k_terminate = 2;
};

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
#include "wait_group.h"

void WaitGroup::Add(int v)
{
    bool notify;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_counter += v;
        if (m_counter < 0) {
            throw std::runtime_error("WaitGroup::Add(): m_counter < 0");
        }
        notify = (m_counter == 0);
    }
    if (notify) {
        m_cond.notify_all();
    }
}

void WaitGroup::Done()
{
    Add(-1);
}

bool WaitGroup::Wait(int64_t timeout_us)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (timeout_us < 0) {
        m_cond.wait(lock, [this]{ return m_counter == 0; });
        return true;
    } else {
        return m_cond.wait_for(lock, std::chrono::microseconds(timeout_us), [this]{ return m_counter == 0; });
    }
}

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
#pragma once

#include <mutex>
#include <condition_variable>

class WaitGroup
{
 public:
    void Add(int v = 1);
    void Done();
    bool Wait(int64_t timeout_us = -1);

 private:
    int m_counter = 0;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

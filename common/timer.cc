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
#include "timer.h"

using namespace std::chrono;

Timer::Timer()
    : m_start(clock::now())
{
}

void Timer::Reset()
{
    m_start = clock::now();
}

int64_t Timer::sec() const
{
    return duration_cast<seconds>(clock::now() - m_start).count();
}

int64_t Timer::ms() const
{
    return duration_cast<milliseconds>(clock::now() - m_start).count();
}

int64_t Timer::us() const
{
    return duration_cast<microseconds>(clock::now() - m_start).count();
}

float Timer::fsec() const
{
    return std::chrono::duration<float>(clock::now() - m_start).count();
}

float Timer::fms() const
{
    return std::chrono::duration<float, std::milli>(clock::now() - m_start).count();
}

float Timer::fus() const
{
    return std::chrono::duration<float, std::micro>(clock::now() - m_start).count();
}

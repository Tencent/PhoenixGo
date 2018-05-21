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
#include "byo_yomi_timer.h"

#include <algorithm>

ByoYomiTimer::ByoYomiTimer()
    : m_enable(false),
      m_remain_time{0.0f, 0.0f},
      m_byo_yomi_time(0.0f),
      m_curr_player(GoComm::BLACK)
{
}

void ByoYomiTimer::Set(float main_time, float byo_yomi_time)
{
    m_enable = true;
    m_remain_time[0] = m_remain_time[1] = main_time;
    m_byo_yomi_time = byo_yomi_time;
    m_timer.Reset();
}

void ByoYomiTimer::Reset()
{
    m_enable = false;
    m_remain_time[0] = m_remain_time[1] = 0.0f;
    m_byo_yomi_time = 0.0f;
    m_curr_player = GoComm::BLACK;
}

bool ByoYomiTimer::IsEnable()
{
    return m_enable;
}

void ByoYomiTimer::HandOff()
{
    m_remain_time[m_curr_player == GoComm::BLACK ? 0 : 1] -= m_timer.fsec();
    m_curr_player = m_curr_player == GoComm::BLACK ? GoComm::WHITE : GoComm::BLACK;
    m_timer.Reset();
}

void ByoYomiTimer::SetRemainTime(GoStoneColor color, float time)
{
    m_remain_time[color == GoComm::BLACK ? 0 : 1] = time;
    if (color == m_curr_player) {
        m_timer.Reset();
    }
}

float ByoYomiTimer::GetRemainTime(GoStoneColor color)
{
    float remain_time = m_remain_time[color == GoComm::BLACK ? 0 : 1];
    if (color == m_curr_player) {
        remain_time -= m_timer.fsec();
    }
    return std::max(remain_time, 0.0f);
}

float ByoYomiTimer::GetByoYomiTime()
{
    return m_byo_yomi_time;
}

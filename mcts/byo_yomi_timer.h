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

#include "common/go_comm.h"
#include "common/timer.h"

class ByoYomiTimer
{
 public:
    ByoYomiTimer();
    void Set(float main_time, float byo_yomi_time);
    void Reset();
    bool IsEnable();
    void HandOff();
    void SetRemainTime(GoStoneColor color, float time);
    float GetRemainTime(GoStoneColor color);
    float GetByoYomiTime();

 private:
    bool m_enable;
    float m_remain_time[2];
    float m_byo_yomi_time;
    GoStoneColor m_curr_player;
    Timer m_timer;
};

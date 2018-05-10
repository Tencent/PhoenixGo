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
#include <thread>

#include "leaky_bucket.h"

LeakyBucket::LeakyBucket(int bucket_size, int refill_period_ms)
    : m_bucket_size(bucket_size), m_tokens(bucket_size),
      m_refill_period(std::chrono::milliseconds(refill_period_ms)),
      m_last_refill(clock::now())
{
}

void LeakyBucket::ConsumeToken()
{
    auto now = clock::now();
    if (now - m_last_refill > m_refill_period) {
        m_last_refill = now;
        m_tokens = m_bucket_size;
    }
    --m_tokens;
}

bool LeakyBucket::Empty()
{
    return m_tokens <= 0;
}

void LeakyBucket::WaitRefill()
{
    if (m_tokens <= 0) {
        std::this_thread::sleep_until(m_last_refill + m_refill_period);
        m_last_refill = clock::now();
        m_tokens = m_bucket_size;
    }
}

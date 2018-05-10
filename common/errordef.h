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

enum {
    ERR_INVALID_INPUT        = -1,
    ERR_FORWARD_TIMEOUT      = -2,

    ERR_READ_CHECKPOINT      = -1000,
    ERR_CREATE_SESSION       = -1001,
    ERR_CREATE_GRAPH         = -1002,
    ERR_RESTORE_VAR          = -1003,
    ERR_SESSION_RUN          = -1005,

    // tensorrt error
    ERR_READ_TRT_MODEL       = -2000,
    ERR_LOAD_TRT_ENGINE      = -2001,
    ERR_CUDA_MALLOC          = -2002,
    ERR_CUDA_FREE            = -2003,
    ERR_CUDA_MEMCPY          = -2004,

    // rpc error
    ERR_GLOBAL_STEP_CONFLICT = -3000,
    ERR_EMPTY_RESP           = -3001,
};

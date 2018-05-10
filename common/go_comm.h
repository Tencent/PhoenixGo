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

#include <inttypes.h>
#include <string>
#include <vector>

// Return code of functions should be "int"
typedef uint8_t GoStoneColor;       // Stone color
typedef int16_t GoCoordId;          // Stone IDs or coordinates
typedef int16_t GoBlockId;          // Block IDs
typedef int16_t GoSize;             // Counts of visit times, used blocks, ..

namespace GoComm {

const GoCoordId BORDER_SIZE = 19;
const GoCoordId GOBOARD_SIZE = BORDER_SIZE * BORDER_SIZE;
const GoCoordId COORD_UNSET = -2;
const GoCoordId COORD_PASS = -1;
const GoCoordId COORD_RESIGN = -3;

const GoSize SIZE_NONE = 0;

const GoBlockId MAX_BLOCK_SIZE = 1 << 8;
const GoBlockId BLOCK_UNSET = -1;

const GoStoneColor EMPTY = 0;
const GoStoneColor BLACK = 1;
const GoStoneColor WHITE = 2;
const GoStoneColor WALL = 3;
const GoStoneColor COLOR_UNKNOWN = -1;
const char *const COLOR_STRING[] = { "Empty", "Black", "White", "Wall" };

const GoCoordId DELTA_X[] = { 0, 1, 0, -1 };
const GoCoordId DELTA_Y[] = { -1, 0, 1, 0 };
const GoSize DELTA_SIZE = sizeof(DELTA_X) / sizeof(*DELTA_X);

const GoSize UINT64_BITS = sizeof(uint64_t) * 8;
const GoSize LIBERTY_STATE_SIZE = (GOBOARD_SIZE + UINT64_BITS - 1) / UINT64_BITS;
const GoSize BOARD_STATE_SIZE = (GOBOARD_SIZE + UINT64_BITS - 1) / UINT64_BITS;

} // namespace GoComm

namespace GoFeature {

const int SIZE_HISTORYEACHSIDE = 16;
const int SIZE_PLAYERCOLOR = 1;

const int STARTPOS_HISTORYEACHSIDE = 0;
const int STARTPOS_PLAYERCOLOR = STARTPOS_HISTORYEACHSIDE + SIZE_HISTORYEACHSIDE;

const int FEATURE_COUNT = STARTPOS_PLAYERCOLOR + SIZE_PLAYERCOLOR;

} // namespace GoFeature


namespace GoFunction {

extern bool InBoard(const GoCoordId id);

extern bool InBoard(const GoCoordId x, const GoCoordId y);

extern bool IsPass(const GoCoordId id);

extern bool IsPass(const GoCoordId x, const GoCoordId y);

extern bool IsUnset(const GoCoordId id);

extern bool IsUnset(const GoCoordId x, const GoCoordId y);

extern bool IsResign(const GoCoordId id);

extern bool IsResign(const GoCoordId x, const GoCoordId y);


extern void IdToCoord(const GoCoordId id, GoCoordId &x, GoCoordId &y);

extern GoCoordId CoordToId(const GoCoordId x, const GoCoordId y);

extern void StrToCoord(const std::string &str, GoCoordId &x, GoCoordId &y);

extern std::string CoordToStr(const GoCoordId x, const GoCoordId y);

extern std::string IdToStr(const GoCoordId id);

extern GoCoordId StrToId(const std::string &str);


extern void CreateGlobalVariables();

extern void CreateHashWeights();

extern void CreateNeighbourCache();

extern void CreateQuickLog2Table();

extern void CreateZobristHash();

} // namespace GoFunction


typedef std::pair<GoCoordId, GoCoordId> GoPosition;
typedef std::pair<uint64_t, uint64_t> GoHashValuePair;

extern GoHashValuePair g_hash_weight[GoComm::BORDER_SIZE][GoComm::BORDER_SIZE];
const GoHashValuePair g_hash_unit(3, 7);
extern uint64_t g_zobrist_board_hash_weight[4][GoComm::GOBOARD_SIZE];
extern uint64_t g_zobrist_ko_hash_weight[GoComm::GOBOARD_SIZE];
extern uint64_t g_zobrist_player_hash_weight[4];

extern std::vector<GoPosition> g_neighbour_cache_by_coord[GoComm::BORDER_SIZE][GoComm::BORDER_SIZE];
extern GoSize g_neighbour_size[GoComm::GOBOARD_SIZE];
extern GoCoordId g_neighbour_cache_by_id[GoComm::GOBOARD_SIZE][GoComm::DELTA_SIZE + 1];
extern GoCoordId g_log2_table[67];

#define FOR_NEI(id, nb) for (GoCoordId *nb = g_neighbour_cache_by_id[(id)]; \
                             GoComm::COORD_UNSET != *nb; ++nb)
#define FOR_EACHCOORD(id) for (GoCoordId id = 0; id < GoComm::GOBOARD_SIZE; ++id)
#define FOR_EACHBLOCK(id) for (GoBlockId id = 0; id < GoComm::MAX_BLOCK_SIZE; ++id)

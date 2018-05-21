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
#include "go_comm.h"

#include <cstring>
#include <mutex>
// #include <glog/logging.h>

#define x first
#define y second

using namespace std;
using namespace GoComm;

GoHashValuePair g_hash_weight[BORDER_SIZE][BORDER_SIZE];

vector<GoPosition> g_neighbour_cache_by_coord[BORDER_SIZE][BORDER_SIZE];
GoSize g_neighbour_size[GOBOARD_SIZE];
GoCoordId g_neighbour_cache_by_id[GOBOARD_SIZE][5];
GoCoordId g_log2_table[67];
uint64_t g_zobrist_board_hash_weight[4][GOBOARD_SIZE];
uint64_t g_zobrist_ko_hash_weight[GOBOARD_SIZE];
uint64_t g_zobrist_player_hash_weight[4];

namespace GoFunction {

bool InBoard(const GoCoordId id) {
    return 0 <= id && id < GOBOARD_SIZE;
}

bool InBoard(const GoCoordId x, const GoCoordId y) {
    return 0 <= x && x < BORDER_SIZE
        && 0 <= y && y < BORDER_SIZE;
}

bool IsPass(const GoCoordId id) {
    return COORD_PASS == id;
}

bool IsPass(const GoCoordId x, const GoCoordId y) {
    return COORD_PASS == CoordToId(x, y);
}

bool IsUnset(const GoCoordId id) {
    return COORD_UNSET == id;
}

bool IsUnset(const GoCoordId x, const GoCoordId y) {
    return COORD_UNSET == CoordToId(x, y);
}

bool IsResign(const GoCoordId id) {
    return COORD_RESIGN == id;
}

bool IsResign(const GoCoordId x, const GoCoordId y) {
    return COORD_RESIGN == CoordToId(x, y);
}


void IdToCoord(const GoCoordId id, GoCoordId &x, GoCoordId &y) {
    if (COORD_PASS == id) {
        x = y = COORD_PASS;
    } else if (COORD_RESIGN == id) {
        x = y = COORD_RESIGN;
    } else if (!InBoard(id)) {
        x = y = COORD_UNSET;
    } else {
        x = id / BORDER_SIZE;
        y = id % BORDER_SIZE;
    }
}

GoCoordId CoordToId(const GoCoordId x, const GoCoordId y) {
    if (COORD_PASS == x && COORD_PASS == y) {
        return COORD_PASS;
    }
    if (COORD_RESIGN == x && COORD_RESIGN == y) {
        return COORD_RESIGN;
    }
    if (!InBoard(x, y)) {
        return COORD_UNSET;
    }
    return x * BORDER_SIZE + y;
}

void StrToCoord(const string &str, GoCoordId &x, GoCoordId &y) {
    // CHECK_EQ(str.length(), 2) << "string[" << str << "] length not equal to 2";
    x = str[0] - 'a';
    y = str[1] - 'a';
    if (str == "zz") {
        x = y = COORD_PASS;
    } else if (!InBoard(x, y)) {
        x = y = COORD_UNSET;
    }
}

string CoordToStr(const GoCoordId x, const GoCoordId y) {
    char buffer[3];
    if (!InBoard(x, y)) {
        buffer[0] = buffer[1] = 'z';
    } else {
        buffer[0] = x + 'a';
        buffer[1] = y + 'a';
    }
    return string(buffer, 2);
}

std::string IdToStr(const GoCoordId id) {
    GoCoordId x, y;
    IdToCoord(id, x, y);
    return CoordToStr(x, y);
}

GoCoordId StrToId(const std::string &str) {
    GoCoordId x, y;
    StrToCoord(str, x, y);
    return CoordToId(x, y);
}

once_flag CreateGlobalVariables_once;
void CreateGlobalVariables() {
    call_once(
        CreateGlobalVariables_once,
        []() {
            CreateNeighbourCache();
            CreateHashWeights();
            CreateQuickLog2Table();
            CreateZobristHash();
        }
    );
}


void CreateHashWeights() {
    g_hash_weight[0][0] = GoHashValuePair(1, 1);
    for (GoCoordId i = 1; i < GOBOARD_SIZE; ++i) {
        g_hash_weight[i / BORDER_SIZE][i % BORDER_SIZE] =
            GoHashValuePair(g_hash_weight[(i - 1) / BORDER_SIZE][(i - 1) % BORDER_SIZE].x * g_hash_unit.x,
                            g_hash_weight[(i - 1) / BORDER_SIZE][(i - 1) % BORDER_SIZE].y * g_hash_unit.y);
    }
}

void CreateNeighbourCache() {
    for (GoCoordId x = 0; x < BORDER_SIZE; ++x) {
        for (GoCoordId y = 0; y < BORDER_SIZE; ++y) {
            GoCoordId id = CoordToId(x, y);

            g_neighbour_cache_by_coord[x][y].clear();
            for (int i = 0; i <= DELTA_SIZE; ++i) {
                g_neighbour_cache_by_id[id][i] = COORD_UNSET;
            }
            for (int i = 0; i < DELTA_SIZE; ++i) {
                GoCoordId nx = x + DELTA_X[i];
                GoCoordId ny = y + DELTA_Y[i];

                if (InBoard(nx, ny)) {
                    g_neighbour_cache_by_coord[x][y].push_back(GoPosition(nx, ny));
                }
            }
            g_neighbour_size[id] = g_neighbour_cache_by_coord[x][y].size();
            for (GoSize i = 0; i < g_neighbour_cache_by_coord[x][y].size(); ++i) {
                g_neighbour_cache_by_id[id][i] = CoordToId(g_neighbour_cache_by_coord[x][y][i].x,
                                                           g_neighbour_cache_by_coord[x][y][i].y);
            }
        }
    }
    // cerr << hex << int(g_neighbour_cache_by_coord) << endl;
}

void CreateQuickLog2Table() {
    memset(g_log2_table, -1, sizeof(g_log2_table));
    int tmp = 1;

    for (GoCoordId i = 0; i < 64; ++i) {
        g_log2_table[tmp] = i;
        tmp *= 2;
        tmp %= 67;
    }
}

#if defined(_WIN32) || defined(_WIN64)
static int rand_r(unsigned int *seed)
{
    unsigned int next = *seed;
    int result;

    next *= 1103515245;
    next += 12345;
    result = (unsigned int)(next / 65536) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int)(next / 65536) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int)(next / 65536) % 1024;

    *seed = next;

    return result;
}
#endif

void CreateZobristHash() {
    uint32_t seed = 0xdeadbeaf;

    for (int i = 0; i < 4; ++i) {
        g_zobrist_player_hash_weight[i] = (uint64_t) rand_r(&seed) << 32 | rand_r(&seed);
        for (int j = 0; j < GOBOARD_SIZE; ++j) {
            g_zobrist_board_hash_weight[i][j] = (uint64_t) rand_r(&seed) << 32 | rand_r(&seed);
        }
    }

    for (int i = 0; i < GOBOARD_SIZE; ++i) {
        g_zobrist_ko_hash_weight[i] = (uint64_t) rand_r(&seed) << 32 | rand_r(&seed);
    }
}

} // namespace GoFunction

#undef y
#undef x

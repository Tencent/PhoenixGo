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
#include "go_state.h"

#include <algorithm>

#define x first
#define y second

using namespace std;
using namespace GoComm;
using namespace GoFunction;
using namespace GoFeature;

// int popcount64(unsigned long long x) {
//     x = (x & 0x5555555555555555ULL) + ((x >> 1) & 0x5555555555555555ULL);
//     x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
//     x = (x & 0x0F0F0F0F0F0F0F0FULL) + ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL);
//     return (x * 0x0101010101010101ULL) >> 56;
// }

GoState::GoState(bool positional_superko) {
    CreateGlobalVariables();

    positional_superko_ = positional_superko;

    FOR_EACHCOORD(i) {
        stones_[i].self_id = i;
    }
    FOR_EACHBLOCK(i) {
        block_pool_[i].in_use = false;
        block_pool_[i].self_id = i;
        block_pool_[i].stones = stones_;
    }
    while (!recycled_blocks_.empty()) {
        recycled_blocks_.pop();
    }
    // CHECK(recycled_blocks_.empty()) << "block reuse util initialize fail";

    feature_history_list_.clear();
    memset(board_state_, 0, sizeof(board_state_));
    memset(visited_positions_, 0, sizeof(visited_positions_));
    memset(legal_move_map_, 1, sizeof(legal_move_map_));
    memset(liberty_count_, 0, sizeof(liberty_count_));
    memset(move_count_, 0, sizeof(move_count_));
    timestamp_ = SIZE_NONE;
    block_in_use_ = SIZE_NONE;
    current_player_ = BLACK;
    ko_position_ = COORD_UNSET;
    last_position_ = COORD_UNSET;
    is_double_pass_ = false;
    board_hash_states_.clear();
    zobrist_hash_value_ = 0;
    // cerr << __func__ << hex << int(g_vtNeighbourCache) << endl;
}


GoState::GoState(const GoState &gms) : GoState() {
    CopyFrom(gms);
}


GoState::~GoState() {
}


GoSize GoState::CalcRegionScore(const GoCoordId &xy, const GoStoneColor color, bool *vis) const {
    // using stack instead of queue for speed up
    stack<GoCoordId> q;
    GoSize cnt = SIZE_NONE;
    GoStoneColor colorSet = EMPTY;
    GoCoordId nxy;

    vis[xy] = true;
    q.push(xy);
    ++cnt;
    while (!q.empty()) {
        nxy = q.top();
        q.pop();
        FOR_NEI(nxy, np) {
            if (vis[*np]) {
                continue;
            }
            colorSet |= board_state_[*np];
            if (EMPTY == board_state_[*np]) {
                vis[*np] = true;
                q.push(*np);
                ++cnt;
            }
        }
    }

    return color != colorSet ? SIZE_NONE : cnt;
}


GoSize GoState::CalcScore(GoSize &black, GoSize &white, GoSize &empty) const {
    CalcScoreWithColor(black, BLACK);
    CalcScoreWithColor(white, WHITE);
    empty = GOBOARD_SIZE - black - white;

    return black - white;
}


void GoState::CalcScoreWithColor(GoSize &cnt, const GoStoneColor color) const {
    bool vis[GOBOARD_SIZE];

    cnt = SIZE_NONE;
    memset(vis, 0, sizeof(vis));
    FOR_EACHCOORD(i) {
        if (vis[i]) {
            continue;
        }
        if (color == board_state_[i]) {
            ++cnt;
        } else if (EMPTY == board_state_[i]) {
            cnt += CalcRegionScore(i, color, vis);
        }
    }
}


GoStoneColor GoState::GetWinner(GoSize &score) const {
    GoSize black, white, empty;
    score = CalcScore(black, white, empty);
    return score > 7.5 ? BLACK : WHITE;
}


GoStoneColor GoState::GetWinner() const {
    GoSize score;
    return GetWinner(score);
}


void GoState::CopyFrom(const GoState &src) {
    *this = src;
    FixBlockInfo();
}


GoCoordId GoState::FindCoord(const GoCoordId id) {
    if (stones_[id].parent_id == stones_[id].self_id) {
        return stones_[id].self_id;
    }
    return stones_[id].parent_id = FindCoord(stones_[id].parent_id);
}


void GoState::FixBlockInfo() {
    FOR_EACHBLOCK(i) {
        block_pool_[i].stones = stones_;
    }
}


bool GoState::IsMovable() const {
    FOR_EACHCOORD(i) {
        if (legal_move_map_[i]) {
            return true;
        }
    }
    return false;
}


GoBlockId GoState::GetBlockIdByCoord(const GoCoordId id) {
    if (EMPTY == board_state_[id]) {
        return BLOCK_UNSET;
    }
    return stones_[FindCoord(id)].block_id;
}


void GoState::GetLastMove(GoCoordId &x, GoCoordId &y) {
    IdToCoord(last_position_, x, y);
}


void GoState::GetNeighbourBlocks(GoBlock &blk, const GoCoordId to, GoBlockId *nbId) {
    nbId[0] = 0;
    blk.SetStoneState(to);
    FOR_NEI(to, nb) {
        blk.SetVirtLiberty(*nb);
        if (EMPTY == board_state_[*nb]) {
            blk.SetLiberty(*nb);
        } else {
            nbId[++nbId[0]] = GetBlockIdByCoord(*nb);
        }
    }
    sort(nbId + 1, nbId + nbId[0] + 1);
    nbId[0] = unique(nbId + 1, nbId + nbId[0] + 1) - nbId - 1;
}


void GoState::GetNewBlock(GoCoordId &id) {
    id = BLOCK_UNSET;
    if (!recycled_blocks_.empty()) {
        id = recycled_blocks_.top();
        recycled_blocks_.pop();
    } else {
        id = block_in_use_++;
        // CHECK(block_in_use_ < MAX_BLOCK_SIZE) << "max-block-size not enough???";
    }
    block_pool_[id].Reset();
}


vector<bool> GoState::GetFeature() const {
    vector<bool> feature(GOBOARD_SIZE * (SIZE_HISTORYEACHSIDE + 1), 0);
    // because push black into history first,
    // if next player is black,
    // we should get planes swap of each two planes
    int reverse_plane = int(Self() == BLACK);

    for (GoSize i = 0; i < SIZE_HISTORYEACHSIDE && i < feature_history_list_.size(); ++i) {
        const string &feature_str = *(feature_history_list_.rbegin() + (i ^ reverse_plane));
        for (int j = 0, k = i; j < GOBOARD_SIZE; ++j, k += 17) {
            feature[k] = feature_str[j] - '0';
        }
    }
    if (Self() == BLACK) {
        for (int j = 0, k = SIZE_HISTORYEACHSIDE; j < GOBOARD_SIZE; ++j, k += 17) {
            feature[k] = 1;
        }
    }

    return feature;
}


const string GoState::GetFeatureString() const {
    string result_string;
    // because push black into history first,
    // if next player is black,
    // we should get planes swap of each two planes
    int reverse_plane = 0;

    reverse_plane = int(Self() == BLACK);
    // CHECK(feature_history_list_.size() % 2 == 0)
    //     << "feature_history_list_ size " << feature_history_list_.size() << " not legal";

    for (GoSize i = 0; i < SIZE_HISTORYEACHSIDE; ++i) {
        if (i < feature_history_list_.size()) {
            result_string += *(feature_history_list_.rbegin() + (i ^ reverse_plane));
        } else {
            result_string += string(GOBOARD_SIZE, '0');
        }
    }
    result_string += string(GOBOARD_SIZE, '0' + (Self() == BLACK));

    return result_string;
}


const string GoState::GetLastFeaturePlane() const {
    return *(feature_history_list_.rbegin() + 1) + *feature_history_list_.rbegin();
}


void GoState::GetSensibleMove() {
    // Add new feature plane
    string plane;
    // black
    {
        plane = "";
        FOR_EACHCOORD(id) {
            if (board_state_[id] == BLACK) {
                plane += "1";
            } else {
                plane += "0";
            }
        }
        feature_history_list_.push_back(plane);
    }
    // white
    {
        plane = "";
        FOR_EACHCOORD(id) {
            if (board_state_[id] == WHITE) {
                plane += "1";
            } else {
                plane += "0";
            }
        }
        feature_history_list_.push_back(plane);
    }

    GoBlockId blkId;
    GoBlockId tmp[2][DELTA_SIZE + 1];
    GoSize libCnt;

    memset(legal_move_map_, 0, sizeof(legal_move_map_));
    GetNewBlock(blkId);

    GoBlock &blk = block_pool_[blkId];

    FOR_EACHCOORD(i) {
        if (EMPTY != board_state_[i]) {
            continue;
        }
        if (ko_position_ == i) {
            continue;
        }
        libCnt = 0;
        FOR_NEI(i, nb) {
            if (EMPTY == board_state_[*nb]) {
                ++libCnt;
                break;
            }
        }
        legal_move_map_[i] = true;
        if (libCnt > 0) {
            continue;
        }
        TryMove(blk, i, tmp[0], tmp[1]);
        blk.CountLiberty();
        if (blk.liberty <= 0) {
            legal_move_map_[i] = false;
            continue;
        }
        // check board state duplicate
        if (positional_superko_) {
            uint64_t new_zobrist_hash_value = zobrist_hash_value_;

            new_zobrist_hash_value ^= g_zobrist_player_hash_weight[Self()];
            new_zobrist_hash_value ^= g_zobrist_player_hash_weight[Opponent()];
            new_zobrist_hash_value ^= g_zobrist_board_hash_weight[Self()][i];

            GoBlockId *dieId = tmp[1];

            for (GoBlockId j = 1; j <= dieId[0]; ++j) {
                FOR_BLOCKSTONE(tid, block_pool_[dieId[j]], {
                    new_zobrist_hash_value ^= g_zobrist_board_hash_weight[Opponent()][tid];
                });
            }

            if (board_hash_states_.find(new_zobrist_hash_value) != board_hash_states_.end()) {
                legal_move_map_[i] = false;
            }
        }
    }
    RecycleBlock(blkId);
}


int GoState::Move(const GoCoordId to) {
    if (!IsLegal(to)) {
        return -1;
    }

    ++timestamp_;
    is_double_pass_ = IsPass(last_position_) && IsPass(to);
    last_position_ = to;
    ko_position_ = COORD_UNSET;

    // update zobrist hash board state
    {
        zobrist_hash_value_ ^= g_zobrist_player_hash_weight[Self()];
        zobrist_hash_value_ ^= g_zobrist_player_hash_weight[Opponent()];
        if (!IsPass(to)) {
            zobrist_hash_value_ ^= g_zobrist_board_hash_weight[Self()][to];
        }
    }

    if (IsPass(to)) {
        HandOff();
        GetSensibleMove();

        if (positional_superko_) {
            board_hash_states_.insert(zobrist_hash_value_);
        }

        return 0;
    }
    GoBlockId blkId, nbId[DELTA_SIZE + 1], dieId[DELTA_SIZE + 1];
    GoSize cnt;
    GetNewBlock(blkId);
    GoBlock &newBlk = block_pool_[blkId];

    if ((cnt = TryMove(newBlk, to, nbId, dieId)) < 0) {
        return -2;
    }
    stones_[to].Reset(blkId);
    newBlk.in_use = true;
    newBlk.head = newBlk.tail = to;
    for (GoBlockId i = 1; i <= nbId[0]; ++i) {
        GoBlock &blk = block_pool_[nbId[i]];

        visited_positions_[nbId[i]] = timestamp_;
        blk.ResetLiberty(to);
        if (Self() == blk.color) {
            newBlk.MergeBlocks(blk);
            RecycleBlock(nbId[i]);
        }
    }
    visited_positions_[blkId] = timestamp_;

    for (GoBlockId i = 1; i <= dieId[0]; ++i) {
        if (1 == block_pool_[dieId[i]].stone_count && 1 == newBlk.stone_count && 1 == newBlk.CountLiberty()) {
            ko_position_ = block_pool_[dieId[i]].head;
        }
        FOR_BLOCKSTONE(tid, block_pool_[dieId[i]], {
            FOR_NEI(tid, nb) {
                if (Self() == board_state_[*nb]) {
                    GoBlockId bid = GetBlockIdByCoord(*nb);

                    visited_positions_[bid] = timestamp_;
                    block_pool_[bid].SetLiberty(tid);
                }
            }
            board_state_[tid] = EMPTY;
            liberty_count_[tid] = SIZE_NONE;

            zobrist_hash_value_ ^= g_zobrist_board_hash_weight[Opponent()][tid];
        });
        RecycleBlock(dieId[i]);
    }

    FOR_USEDBLOCK(i) {
        if (block_pool_[i].in_use && timestamp_ == visited_positions_[i]) {
            GoBlock &blk = block_pool_[i];
            blk.CountLiberty();
            FOR_BLOCKSTONE(tid, blk, {
                liberty_count_[tid] = blk.liberty;
            });

        }
    }
    ++move_count_[to];
    board_state_[to] = Self();

    HandOff();
    GetSensibleMove();

    if (positional_superko_) {
        board_hash_states_.insert(zobrist_hash_value_);
    }

    return 0;
}


int GoState::Move(const GoCoordId x, const GoCoordId y) {
    return Move(CoordToId(x, y));
}


void GoState::RecycleBlock(const GoBlockId id) {
    block_pool_[id].in_use = false;
    recycled_blocks_.push(id);
}


void GoState::ShowBoard(bool bNoColor) const {
    GoCoordId y;

    fprintf(stderr, "current player: %d\n", current_player_);
    FOR_EACHCOORD(i) {
        y = i % BORDER_SIZE;
        if (!bNoColor) {
            if (BLACK == board_state_[i]) {
                fprintf(stderr, "\033[31m");
            } else if (WHITE == board_state_[i]) {
                fprintf(stderr, "\033[33m");
            }
        }
        if (i == last_position_) {
            fprintf(stderr, "X%c", BORDER_SIZE - 1 != y ? ' ' : '\n');
        } else {
            fprintf(stderr, "%d%c", int(board_state_[i]), BORDER_SIZE - 1 != y ? ' ' : '\n');
        }
        if (!bNoColor) {
            fprintf(stderr, "\033[0m");
        }
    }
    fprintf(stderr, "\n");
}


void GoState::ShowLibCount() const {
    fprintf(stderr, "<-------------  lib  --------------->\n");
    for (GoCoordId x = 0; x < BORDER_SIZE; ++x) {
        for (GoCoordId y = 0; y < BORDER_SIZE; ++y) {
            fprintf(stderr, "%d ", int(liberty_count_[CoordToId(x, y)]));
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "-------------------------------------\n");
}


void GoState::ShowState() {
    fprintf(stderr, "block in used: %d", int(block_in_use_));
    FOR_USEDBLOCK(i) {
        fprintf(stderr, "id %d: color %d, x %d y %d, stone %d lib %d\n",
                int(i), int(block_pool_[i].color), int(block_pool_[i].head / BORDER_SIZE),
                int(block_pool_[i].head % BORDER_SIZE), int(block_pool_[i].stone_count),
                int(block_pool_[i].CountLiberty()));
    }
}


void GoState::ShowLegalMap() const {
    fprintf(stderr, "<------------- legal --------------->\n");
    for (GoCoordId x = 0; x < BORDER_SIZE; ++x) {
        for (GoCoordId y = 0; y < BORDER_SIZE; ++y) {
            fprintf(stderr, "%d ", int(legal_move_map_[CoordToId(x, y)]));
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "-------------------------------------\n");
}


GoSize GoState::TryMove(GoBlock &blk, const GoCoordId to, GoBlockId *nbId, GoBlockId *dieId, GoCoordId libNeed) {
    if (!legal_move_map_[to]) {
        return -1;
    }
    GoSize cnt = 0;

    blk.Reset();
    blk.stone_count = 1;
    blk.color = Self();
    GetNeighbourBlocks(blk, to, nbId);

    dieId[0] = 0;

    for (GoBlockId i = 1; i <= nbId[0]; ++i) {
        GoBlock &tmp = block_pool_[nbId[i]];
        if (Self() != tmp.color) {
            if (1 == tmp.CountLiberty()) {
                cnt += tmp.stone_count;
                dieId[++dieId[0]] = nbId[i];
            }
        } else {
            blk.TryMergeBlocks(tmp);
        }
    }
    blk.ResetLiberty(to);
    if (blk.CountLiberty() >= libNeed) {
        return cnt;
    }
    if (SIZE_NONE != dieId[0]) {
        for (GoBlockId i = 1; i <= dieId[0]; ++i) {
            const GoBlock &tmp = block_pool_[dieId[i]];
            for (GoSize k = 0; k < LIBERTY_STATE_SIZE; ++k) {
                blk.liberty_state[k] |= tmp.stone_state[k];
                blk.liberty_state[k] &= blk.virtual_liberty_state[k];
            }
        }
    }

    return cnt;
}

uint64_t GoState::GetNewHashValue(GoCoordId to)
{
    if (to == COORD_PASS) {
        uint64_t new_zobrist_hash_value = zobrist_hash_value_;
        new_zobrist_hash_value ^= g_zobrist_player_hash_weight[Self()];
        new_zobrist_hash_value ^= g_zobrist_player_hash_weight[Opponent()];
        return new_zobrist_hash_value;
    }

    GoBlockId blkId;
    GoBlockId tmp[2][DELTA_SIZE + 1];

    GetNewBlock(blkId);

    GoBlock &blk = block_pool_[blkId];
    TryMove(blk, to, tmp[0], tmp[1]);

    uint64_t new_zobrist_hash_value = zobrist_hash_value_;

    new_zobrist_hash_value ^= g_zobrist_player_hash_weight[Self()];
    new_zobrist_hash_value ^= g_zobrist_player_hash_weight[Opponent()];
    new_zobrist_hash_value ^= g_zobrist_board_hash_weight[Self()][to];

    GoBlockId *dieId = tmp[1];

    for (GoBlockId j = 1; j <= dieId[0]; ++j) {
        FOR_BLOCKSTONE(tid, block_pool_[dieId[j]], {
            new_zobrist_hash_value ^= g_zobrist_board_hash_weight[Opponent()][tid];
        });
    }

    RecycleBlock(blkId);
    return new_zobrist_hash_value;
}

#undef y
#undef x

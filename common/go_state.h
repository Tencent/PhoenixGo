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

#include <cstdlib>
#include <cstring>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

// #include <glog/logging.h>
#include "go_comm.h"

#if defined(_WIN32) || defined(_WIN64)
#  include <intrin.h>
#  define __builtin_popcount __popcnt
#  define __builtin_popcountll __popcnt64
#endif

template<class IntType>
inline IntType Lowbit(const IntType &x) {
    return x & (-x);
}

#define FOR_BLOCKSTONE(id, blk, loop_body) { \
    GoCoordId id = (blk).head; \
    while (true) { \
        loop_body \
        if (id == stones_[id].next_id) { \
            break; \
        } \
        id = stones_[id].next_id; \
    } \
}

#define FOR_USEDBLOCK(id) \
    for (GoBlockId id = 0; id < block_in_use_; ++id)
#define FOR_BLOCKLIB(id, blk) \
    for (GoCoordId id = (blk).LibBegin(); (blk).LibEnd() != id; id = (blk).LibNext(id))


int popcount64(unsigned long long x);

struct GoStone {
    GoCoordId self_id;      // Id of this stone
    GoBlockId block_id;     // Id of its block
    GoCoordId next_id;      // Use like link-list
    GoCoordId parent_id;    // Use like union-find-set (rooted by tail)

    // inline void SetSelfId(GoCoordId id) { self_id = id; }

    inline void Reset(GoCoordId id = GoComm::COORD_UNSET) {
        next_id = parent_id = self_id;
        block_id = id;
    }
} ;

struct GoBlock {
    GoBlockId self_id;          // Id of this block
    GoStoneColor color;         // Color of this block
    GoSize liberty;             // Last liberty count
    GoCoordId head, tail;       // Stones record
    GoSize stone_count;         // Stone count
    GoStone *stones;            // Stone detail
    bool in_use;                // Block is in use

    uint64_t liberty_state[GoComm::LIBERTY_STATE_SIZE];             // Liberties
    uint64_t virtual_liberty_state[GoComm::LIBERTY_STATE_SIZE];     // Virtual liberties
    uint64_t stone_state[GoComm::BOARD_STATE_SIZE];                 // Stones

    // inline void SetSelfId(int id) { self_id = id; }

    GoCoordId LibBegin() const {
        for (GoSize i = 0; i < GoComm::LIBERTY_STATE_SIZE; ++i) {
            if (0 == liberty_state[i]) {
                continue;
            }
            return g_log2_table[Lowbit(liberty_state[i]) % 67] + (i << 6);
        }
        return LibEnd();
    }

    GoCoordId LibEnd() const { return GoComm::GOBOARD_SIZE; }

    GoCoordId LibNext(const int id) const {
        // & 63 to find pos, + 1 to pass corresponding pos
        uint64_t val = 0;
        if (63 != (id & 63)) {
            val = liberty_state[id >> 6] & (uint64_t(-1) << ((id & 63) + 1));
        }
        for (GoSize i = id >> 6; i < GoComm::LIBERTY_STATE_SIZE; ) {
            if (0 == val) {
                ++i;
                val = liberty_state[i];
            } else {
                return g_log2_table[Lowbit(val) % 67] + (i << 6);
            }
        }
        return LibEnd();
    }

    void Reset() {
        memset(liberty_state, 0, sizeof(liberty_state));
        memset(virtual_liberty_state, 0, sizeof(virtual_liberty_state));
        memset(stone_state, 0, sizeof(stone_state));
        in_use = false;
        head = tail = GoComm::COORD_UNSET;
        liberty = stone_count = GoComm::SIZE_NONE;
    }

    inline void SetLiberty(const GoCoordId id) {
        liberty_state[id >> 6] |= 1ULL << (id & 63);
    }

    inline void SetVirtLiberty(const GoCoordId id) {
        virtual_liberty_state[id >> 6] |= 1ULL << (id & 63);
    }

    inline void SetStoneState(const GoCoordId id) {
        stone_state[id >> 6] |= 1ULL << (id & 63);
    }

    inline void ResetLiberty(const GoCoordId id) {
        liberty_state[id >> 6] &= ~(1ULL << (id & 63));
    }

    inline void ResetVirtLiberty(const GoCoordId id) {
        virtual_liberty_state[id >> 6] &= ~(1ULL << (id & 63));
    }

    inline void ResetStoneState(const GoCoordId id) {
        stone_state[id >> 6] &= ~(1ULL << (id & 63));
    }

    inline bool GetLiberty(const GoCoordId id) const {
        return liberty_state[id >> 6] & (1ULL << (id & 63));
    }

    GoCoordId GetLowestLiberty() const {
        for (GoSize i = 0; i < GoComm::LIBERTY_STATE_SIZE; ++i) {
            const uint64_t &st = liberty_state[i];
            if (0 != st) {
                return g_log2_table[Lowbit(st) % 67] + (i << 6);
            }
        }
        // CHECK(false) << "Wow~ A block without liberty found! stone size: " << stone_count
        // << "\tstone color: " << GoComm::COLOR_STR[color];
        return GoComm::COORD_UNSET;
    }

    bool IsNoLiberty() {
        return GoComm::SIZE_NONE == this->CountLiberty();
    }

    inline GoSize CountLiberty() {
        return liberty = __builtin_popcountll(liberty_state[0]) +
            __builtin_popcountll(liberty_state[1]) +
            __builtin_popcountll(liberty_state[2]) +
            __builtin_popcountll(liberty_state[3]) +
            __builtin_popcountll(liberty_state[4]) +
            __builtin_popcountll(liberty_state[5]);
    }

    GoStone *GetHead() const {
        return this->stones + this->head;
    }

    GoStone *GetTail() const {
        return this->stones + this->tail;
    }

    // for merging block-a into this
    int TryMergeBlocks(const GoBlock &a) {
        // CHECK(GoComm::SIZE_NONE != a.stone_count) << "block \"a\" is empty";
        // CHECK(GoComm::SIZE_NONE != this->stone_count) << "block \"this\" is empty";
        // CHECK_NE(this->self_id, a.self_id) << "Trying to merge the same block, id " << this->self_id;
        // merge liberty
        this->stone_count += a.stone_count;
        for (GoSize i = 0; i < GoComm::LIBERTY_STATE_SIZE; ++i) {
            this->stone_state[i] |= a.stone_state[i];
            this->virtual_liberty_state[i] |= a.virtual_liberty_state[i];
            this->virtual_liberty_state[i] &= ~this->stone_state[i];
            this->liberty_state[i] |= a.liberty_state[i];
            this->liberty_state[i] &= this->virtual_liberty_state[i];
        }
        return 0;
    }

    void MergeBlocks(const GoBlock &a) {
        // CHECK(GoComm::SIZE_NONE != a.stone_count) << "block \"a\" is empty";
        // CHECK(GoComm::SIZE_NONE != this->stone_count) << "block \"this\" is empty";
        // CHECK_NE(this->self_id, a.self_id) << "Trying to merge the same block, id " << this->self_id;
        // merge stones
        a.GetTail()->next_id = this->GetHead()->self_id;
        this->head = a.head;
        a.GetTail()->parent_id = this->GetTail()->self_id;
    }
} ;

class GoState {
 public:
    GoState(bool positional_superko = true);

    GoState(const GoState &gms);

    ~GoState();


    // basic functions
    GoSize CalcScore(GoSize &black, GoSize &white, GoSize &empty) const;

    GoStoneColor GetWinner(GoSize &score) const;

    GoStoneColor GetWinner() const;

    void CopyFrom(const GoState &src);

    inline GoStoneColor CurrentPlayer() const { return current_player_; }

    inline GoCoordId GetLastMove() const { return last_position_; }

    void GetLastMove(GoCoordId &x, GoCoordId &y);

    GoSize GetLibertyByCoor(const GoCoordId x, const GoCoordId y) const {
        return GetLibertyById(GoFunction::CoordToId(x, y));
    }

    inline GoSize GetLibertyById(const GoCoordId id) const { return liberty_count_[id]; }

    inline void HandOff() { current_player_ = Opponent(); }

    bool IsDoublePass() const { return is_double_pass_; }

    inline bool IsLegal(const GoCoordId id) const { return GoFunction::IsPass(id) || legal_move_map_[id]; }
    // inline bool IsLegal(const GoCoordId id) const { return legal_move_map_[id]; }

    bool IsLegal(const GoCoordId x, const GoCoordId y) { return IsLegal(GoFunction::CoordToId(x, y)); }

    bool IsMovable() const;

    int Move(const GoCoordId id);

    int Move(const GoCoordId x, const GoCoordId y);

    inline GoStoneColor Opponent(const GoStoneColor color = GoComm::COLOR_UNKNOWN) const {
        return GoComm::BLACK + GoComm::WHITE
            - (GoComm::COLOR_UNKNOWN != color ? color : current_player_);
    }

    inline GoStoneColor Self() const { return current_player_; }

    GoSize TryMove(GoBlock &blk, const GoCoordId id, GoBlockId *nbId, GoBlockId *dieId, GoSize libCnt = GoComm::GOBOARD_SIZE); // nbId, dieId: size should be at least 5


    // features for rollout
    const GoStoneColor *GetBoard() const { return board_state_; }

    const bool *GetLegal() const { return legal_move_map_; }

    const GoSize *GetLib() const { return liberty_count_; }

    const GoSize *GetMoveCount() const { return move_count_; }

    const GoSize &GetTs() const { return timestamp_; };

    std::vector<bool> GetFeature() const;

    const std::string GetFeatureString() const;

    const std::string GetLastFeaturePlane() const;


    // for lgr
    const GoBlock& GetBlockById(GoCoordId id) {
        GoBlockId blk_id = stones_[FindCoord(id)].block_id;
        return block_pool_[blk_id];
    }

    // for debug
    void ShowBoard(bool bNoColor = false) const;

    void ShowLibCount() const;

    void ShowState();

    void ShowLegalMap() const;

    uint64_t GetHashValue() const { return zobrist_hash_value_; }

    uint64_t GetNewHashValue(GoCoordId to);


 protected:
    GoSize CalcRegionScore(const GoCoordId &xy, const GoStoneColor color, bool *vis) const;

    void CalcScoreWithColor(GoSize &cnt, const GoStoneColor color) const;

    GoCoordId FindCoord(const GoCoordId id);

    void FixBlockInfo();

    GoSize GetLibertyByBlock(const GoBlock &blk) { return GetLibertyById(blk.head); }

    GoBlockId GetBlockIdByCoord(const GoCoordId id);

    void GetNeighbourBlocks(GoBlock &blk, const GoCoordId to, GoBlockId *nbId);

    void GetSensibleMove();


 protected:
    // need RecycleBlock after used!!!
    void GetNewBlock(GoBlockId &id);

    void RecycleBlock(const GoBlockId id);


 protected:
    // board utils
    GoBlock block_pool_[GoComm::MAX_BLOCK_SIZE];
    std::stack<GoBlockId> recycled_blocks_;
    GoSize block_in_use_;
    GoStone stones_[GoComm::GOBOARD_SIZE];
    GoStoneColor board_state_[GoComm::GOBOARD_SIZE];
    GoStoneColor current_player_;
    GoCoordId last_position_;
    GoCoordId ko_position_;
    bool is_double_pass_;
    bool positional_superko_;

    // hash board state
    std::unordered_set<uint64_t> board_hash_states_;
    uint64_t zobrist_hash_value_;

    // visit status
    GoSize timestamp_;
    GoSize visited_positions_[GoComm::MAX_BLOCK_SIZE];

    // features
    bool legal_move_map_[GoComm::GOBOARD_SIZE];
    GoSize liberty_count_[GoComm::GOBOARD_SIZE];
    GoSize move_count_[GoComm::GOBOARD_SIZE];

    std::vector<std::string> feature_history_list_;
} ;


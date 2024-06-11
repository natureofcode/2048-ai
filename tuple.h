#ifndef TUPLE_H
#define TUPLE_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <memory>
#include <numeric>
#include <thread>

#include "board.h"
#include "tuple_move.h"
#include "array.h"

/*
 T0  T1  T2  T3
 T4  T5  T6  T7
 T8  T9 T10 T11
T12 T13 T14 T15
*/
template <int T0, int T1, int T2, int T3, int T4, int T5, int T6, int T7,
          int T8, int T9, int T10, int T11, int T12, int T13, int T14, int T15>
class Tuple {
 public:
  static const long kNumTuples = long(T0) * T1 * T2 * T3 * T4 * T5 * T6 * T7 *
                                 T8 * T9 * T10 * T11 * T12 * T13 * T14 * T15;
  static const int kNumTiles = (T0 > 1) + (T1 > 1) + (T2 > 1) + (T3 > 1) +
                               (T4 > 1) + (T5 > 1) + (T6 > 1) + (T7 > 1) +
                               (T8 > 1) + (T9 > 1) + (T10 > 1) + (T11 > 1) +
                               (T12 > 1) + (T13 > 1) + (T14 > 1) + (T15 > 1);
  static const int kMaxAnchorRank;
  static const char kTupleFile[];

  Tuple(double save_threshold) : tuple_moves(0xff), save_threshold(save_threshold) {
    tuple_moves.Load(kTupleFile);
  }

  ~Tuple() {
    auto is_valid = [](const TupleMove& t) { return t.ValidProb() && t.Prob() > 0; };
    tuple_moves.Save(kTupleFile, is_valid, save_threshold);
  }

  float SuggestMove(const Board& board, int* move) {
    static constexpr int rotated_moves[4][4] = {
        // up left right down
        {0, 1, 2, 3},
        {1, 3, 0, 2},
        {3, 2, 1, 0},
        {2, 0, 3, 1},
    };

    TupleBoard b(board, tuple_moves);
    float max_prob = 0;
    for (int d = 0; d < 4; ++d) {
      if (d > 0) b.Rotate();
      int m = 0;
      auto prob = b.SuggestMove(&m);
      if (max_prob < prob) {
        max_prob = prob;
        *move = rotated_moves[d][m];
      }
    }
    return max_prob;
  }

  float Lookup(int v, int* m) const {
    *m = tuple_moves[v].move;
    return tuple_moves[v].Prob();
  }

  void Show() const {
    for (long i = 0; i < kNumTuples; ++i) {
      int move;
      auto prob = Lookup(i, &move);
      if (prob == 0) continue;

      int tile[kNumTiles];
      Decode(i, tile);

      printf("%d", tile[0] ? 1 << tile[0] : 0);
      for (int t = 1; t < kNumTiles; ++t)
        printf(" %d", tile[t] ? 1 << tile[t] : 0);
      printf(", %s, %f\n", Board::move_names[move], prob);
    }
  }

  void Query(int index[]) const {
    TupleBoard b;
    b.SetSmallTiles(index);
    auto t = tuple_moves[b.CompactSmallTiles()];
    printf("%s %.3f\n", t.Prob() ? Board::move_names[t.move] : ":(", t.Prob());
  }

 private:
  class TupleBoard : public Board {
   public:
    TupleBoard() = default;
    TupleBoard(Array<TupleMove, kNumTuples>& tuple_moves) : tuple_moves(tuple_moves) {}
    TupleBoard(const Board& b, Array<TupleMove, kNumTuples>& tuple_moves)
        : Board(b), tuple_moves(tuple_moves) {}

    void Prefill() {
      int max_rank = 15;
      if (T0 == 1) board[0][0] = max_rank--;
      if (T1 == 1) board[1][0] = max_rank--;
      if (T2 == 1) board[2][0] = max_rank--;
      if (T3 == 1) board[3][0] = max_rank--;
      if (T4 == 1) board[0][1] = max_rank--;
      if (T5 == 1) board[1][1] = max_rank--;
      if (T6 == 1) board[2][1] = max_rank--;
      if (T7 == 1) board[3][1] = max_rank--;
      if (T8 == 1) board[0][2] = max_rank--;
      if (T9 == 1) board[1][2] = max_rank--;
      if (T10 == 1) board[2][2] = max_rank--;
      if (T11 == 1) board[3][2] = max_rank--;
      if (T12 == 1) board[0][3] = max_rank--;
      if (T13 == 1) board[1][3] = max_rank--;
      if (T14 == 1) board[2][3] = max_rank--;
      if (T15 == 1) board[3][3] = max_rank--;
    }

    void SetSmallTiles(int tile[]) {
      int i = 0;
      if (T0 > 1) board[0][0] = tile[i++];
      if (T1 > 1) board[1][0] = tile[i++];
      if (T2 > 1) board[2][0] = tile[i++];
      if (T3 > 1) board[3][0] = tile[i++];
      if (T4 > 1) board[0][1] = tile[i++];
      if (T5 > 1) board[1][1] = tile[i++];
      if (T6 > 1) board[2][1] = tile[i++];
      if (T7 > 1) board[3][1] = tile[i++];
      if (T8 > 1) board[0][2] = tile[i++];
      if (T9 > 1) board[1][2] = tile[i++];
      if (T10 > 1) board[2][2] = tile[i++];
      if (T11 > 1) board[3][2] = tile[i++];
      if (T12 > 1) board[0][3] = tile[i++];
      if (T13 > 1) board[1][3] = tile[i++];
      if (T14 > 1) board[2][3] = tile[i++];
      if (T15 > 1) board[3][3] = tile[i++];
    }

    long CompactSmallTiles() const {
      long v = 0;
      if (T0 > 1) v = v * T0 + board[0][0];
      if (T1 > 1) v = v * T1 + board[1][0];
      if (T2 > 1) v = v * T2 + board[2][0];
      if (T3 > 1) v = v * T3 + board[3][0];
      if (T4 > 1) v = v * T4 + board[0][1];
      if (T5 > 1) v = v * T5 + board[1][1];
      if (T6 > 1) v = v * T6 + board[2][1];
      if (T7 > 1) v = v * T7 + board[3][1];
      if (T8 > 1) v = v * T8 + board[0][2];
      if (T9 > 1) v = v * T9 + board[1][2];
      if (T10 > 1) v = v * T10 + board[2][2];
      if (T11 > 1) v = v * T11 + board[3][2];
      if (T12 > 1) v = v * T12 + board[0][3];
      if (T13 > 1) v = v * T13 + board[1][3];
      if (T14 > 1) v = v * T14 + board[2][3];
      if (T15 > 1) v = v * T15 + board[3][3];
      return v;
    }

    bool IsRegular() const {
      // Top-left 3x2 configuration:
      // (0,0) != (1,0) != (2,0)
      //  !=       !=        V
      // (0,1) >  (1,1) >  (2,1)
      // plus (1,1) != (2,0)
#ifdef BIG_TUPLES
      if (board[0][0] == 0 || board[1][0] == 0) return false;
#endif
      if (board[0][0] == board[1][0] || board[1][0] == board[2][0]) return false;
      if (board[0][1] <= board[1][1] || board[1][1] <= board[2][1]) return false;
      if (board[0][0] == board[0][1]) return false;
      if (board[1][0] == board[1][1]) return false;
      if (board[2][0] <= board[2][1]) return false;
      if (board[1][1] == board[2][0]) return false;

      // Bottom two rows and rightmost column are smaller than anchor rank.
      int anchor_rank =
        std::min(kMaxAnchorRank, std::min(board[1][1], board[2][0]));
      if (board[2][1] > kMaxAnchorRank) return false;
      if (board[3][0] > anchor_rank - 1) return false;
      if (board[3][1] > anchor_rank - 1) return false;
      if (board[0][2] > anchor_rank - 1) return false;
      if (board[1][2] > anchor_rank - 1) return false;
      if (board[2][2] > anchor_rank - 1) return false;
      if (board[3][2] > anchor_rank - 1) return false;
      if (board[0][3] > anchor_rank - 2) return false;
      if (board[1][3] > anchor_rank - 2) return false;
      if (board[2][3] > anchor_rank - 2) return false;
      if (board[3][3] > anchor_rank - 2) return false;
      return true;
    }

    bool IsGoal() const {
      int anchor_rank =
        std::min(kMaxAnchorRank, std::min(board[1][1], board[2][0]) - 1);
      return board[2][1] == anchor_rank &&
        (
#ifdef BIG_TUPLES
         board[3][0] == anchor_rank || board[3][1] == anchor_rank ||
         board[0][2] == anchor_rank || board[1][2] == anchor_rank ||
         board[2][2] == anchor_rank || board[3][2] == anchor_rank ||
#endif
         (board[3][1] == anchor_rank - 1 &&
          (board[3][0] == anchor_rank - 1 ||
           board[3][2] == anchor_rank - 1)) ||
         (board[3][0] == anchor_rank - 1 && board[3][1] == anchor_rank - 2 &&
          board[3][2] == anchor_rank - 2) ||
         (board[2][2] == anchor_rank - 1 &&
          (board[1][2] == anchor_rank - 1 ||
           board[2][3] == anchor_rank - 1 ||
           board[3][2] == anchor_rank - 1)));
    }

    bool HasSameTops(const TupleBoard& parent) const {
      if (T0 == 1 && board[0][0] != parent.board[0][0]) return false;
      if (T1 == 1 && board[1][0] != parent.board[1][0]) return false;
      if (T2 == 1 && board[2][0] != parent.board[2][0]) return false;
      if (T3 == 1 && board[3][0] != parent.board[3][0]) return false;
      if (T4 == 1 && board[0][1] != parent.board[0][1]) return false;
      if (T5 == 1 && board[1][1] != parent.board[1][1]) return false;
      if (T6 == 1 && board[2][1] != parent.board[2][1]) return false;
      if (T7 == 1 && board[3][1] != parent.board[3][1]) return false;
      if (T8 == 1 && board[0][2] != parent.board[0][2]) return false;
      if (T9 == 1 && board[1][2] != parent.board[1][2]) return false;
      if (T10 == 1 && board[2][2] != parent.board[2][2]) return false;
      if (T11 == 1 && board[3][2] != parent.board[3][2]) return false;
      if (T12 == 1 && board[0][3] != parent.board[0][3]) return false;
      if (T13 == 1 && board[1][3] != parent.board[1][3]) return false;
      if (T14 == 1 && board[2][3] != parent.board[2][3]) return false;
      if (T15 == 1 && board[3][3] != parent.board[3][3]) return false;
      return true;
    }

    float TryTiles() {
      int empty_tiles = 0;
      float tile2_prob = 0, tile4_prob = 0;
      for (int y = 2; y < N; ++y) {
        for (int x = 0; x < N - 1; ++x) {
          if (board[x][y]) continue;
          ++empty_tiles;
          board[x][y] = 1;
          tile2_prob += TryMoves();
          board[x][y] = 2;
          tile4_prob += TryMoves();
          board[x][y] = 0;
        }
      }
      int x = 3;
      for (int y = 0; y < N; ++y) {
        if (board[x][y]) continue;
        ++empty_tiles;
        board[x][y] = 1;
        tile2_prob += TryMoves();
        board[x][y] = 2;
        tile4_prob += TryMoves();
        board[x][y] = 0;
      }
      return (tile2_prob * (1.0 - options.tile4_prob) + tile4_prob * options.tile4_prob) / empty_tiles;
    }

    float TryMoves() {
      auto v = CompactSmallTiles();
      if (!tuple_moves[v].ValidProb()) {
        float max_prob = 0;
        int max_move = 0;
        for (int m = 0; m < 4; ++m) {
          float prob;
          TupleBoard n = *this;
          if (!(n.*moves[m])() || !n.IsRegular() || !n.HasSameTops(*this))
            prob = 0;
          else if (n.IsGoal())
            prob = 1;
          else
            prob = n.TryTiles();
          if (max_prob < prob) {
            max_prob = prob;
            max_move = m;
          }
        }
        tuple_moves[v].move = max_move;
        tuple_moves[v].SetProb(max_prob);

        ShowProgress();
      }
      return tuple_moves[v].Prob();
    }

    // Rotate the board 90-degree clockwise.
    void Rotate() {
      int rotated[N][N];
      for (int x = 0; x < N; ++x)
        for (int y = 0; y < N; ++y) rotated[N - 1 - y][x] = board[x][y];
      for (int x = 0; x < N; ++x)
        for (int y = 0; y < N; ++y) board[x][y] = rotated[x][y];
    }

    void Transpose() {
      for (int x = 0; x < N; ++x)
        for (int y = x + 1; y < N; ++y) std::swap(board[x][y], board[y][x]);
    }

    float SuggestMove(int* move) {
      for (int i = 0; i < 2; ++i) {
        bool transposed = i;
        if (IsRegular() && !IsGoal()) {
          auto v = CompactSmallTiles();
          {
            // Don't use tuple_move pointer after Compute as it may be invalidated.
            auto tuple_move = tuple_moves.locate(v);
            if (!tuple_move || !tuple_move->ValidProb()) Compute();
          }
          *move = tuple_moves[v].move;
          if (transposed) {
            *move = (*move < 2 ? 1 : 5) - *move;
            Transpose();
          }
          return tuple_moves[v].Prob();
        }
        Transpose();
      }
      return 0;
    }

   private:
    void Compute() {
      TupleBoard b(*this, tuple_moves);
      b.Prefill();
      assert(b.IsRegular());
      assert(!b.IsGoal());
      printf(" ");
      b.TryMoves();
      printf("\b");
    }

    Array<TupleMove, kNumTuples>& tuple_moves;
  };

  template <int M>
  int Decode(long* v) const {
    int field = *v % M;
    *v /= M;
    return field;
  }

  void Decode(long v, int tile[]) const {
    int i = kNumTiles - 1;
    if (T15 > 1) tile[i--] = Decode<T15>(&v);
    if (T14 > 1) tile[i--] = Decode<T14>(&v);
    if (T13 > 1) tile[i--] = Decode<T13>(&v);
    if (T12 > 1) tile[i--] = Decode<T12>(&v);
    if (T11 > 1) tile[i--] = Decode<T11>(&v);
    if (T10 > 1) tile[i--] = Decode<T10>(&v);
    if (T9 > 1) tile[i--] = Decode<T9>(&v);
    if (T8 > 1) tile[i--] = Decode<T8>(&v);
    if (T7 > 1) tile[i--] = Decode<T7>(&v);
    if (T6 > 1) tile[i--] = Decode<T6>(&v);
    if (T5 > 1) tile[i--] = Decode<T5>(&v);
    if (T4 > 1) tile[i--] = Decode<T4>(&v);
    if (T3 > 1) tile[i--] = Decode<T3>(&v);
    if (T2 > 1) tile[i--] = Decode<T2>(&v);
    if (T1 > 1) tile[i--] = Decode<T1>(&v);
    if (T0 > 1) tile[i--] = Decode<T0>(&v);
  }

  static void ShowProgress() {
    static long progress_count = 0, progress_check = 1 << 20;
    if (++progress_count >= progress_check) {
      progress_check += 1 << 20;
      printf("\b%c", "-\\|/"[(progress_count >> 20) % 4]);
      fflush(stdout);
    }
  }

  Array<TupleMove, kNumTuples> tuple_moves;
  double save_threshold;
};

using Tuple10 = Tuple<1, 1, 1, 10,  // row 0
                      1, 1, 1, 9,   // row 1
                      10, 9, 8, 8,  // row 2
                      8, 8, 8, 8>;  // row 3

template <>
const int Tuple10::kMaxAnchorRank = 10;
template <>
const char Tuple10::kTupleFile[] = "tuple_moves.10a";

template <>
bool Tuple10::TupleBoard::IsRegular() const {
  // Top-left 3x2 configuration:
  // (0,0) != (1,0) != (2,0) > (3,0)
  //  !=       !=       !=
  // (0,1) != (1,1) != (2,1)
#ifdef BIG_TUPLES
  if (board[0][0] == 0 || board[1][0] == 0) return false;
#endif
  if (board[0][0] == board[1][0] || board[1][0] == board[2][0]) return false;
  if (board[0][1] == board[1][1] || board[1][1] == board[2][1]) return false;
  if (board[0][0] == board[0][1]) return false;
  if (board[1][0] == board[1][1]) return false;
#ifdef BIG_TUPLES
  if (board[2][0] == board[2][1] || board[2][0] <= board[3][0]) return false;
#else
  if (board[2][0] <= board[2][1]) return false;
#endif

  // Bottom two rows and rightmost column are smaller than anchor rank.
  int anchor_rank = std::min(board[0][1], std::min(board[1][1], board[2][1]));
  if (anchor_rank > kMaxAnchorRank) return false;
  if (board[3][0] > anchor_rank - 1) return false;
  if (board[3][1] > anchor_rank - 2) return false;
  if (board[0][2] > anchor_rank - 1) return false;
  if (board[1][2] > anchor_rank - 2) return false;
  if (board[2][2] > anchor_rank - 3) return false;
  if (board[3][2] > anchor_rank - 3) return false;
  if (board[0][3] > anchor_rank - 3) return false;
  if (board[1][3] > anchor_rank - 3) return false;
  if (board[2][3] > anchor_rank - 3) return false;
  if (board[3][3] > anchor_rank - 3) return false;
  return true;
}

template <>
bool Tuple10::TupleBoard::IsGoal() const {
  int anchor_rank = std::min(board[0][1], std::min(board[1][1], board[2][1]));
  return (board[0][2] == anchor_rank - 1 && board[1][2] == anchor_rank - 2 &&
          ((board[2][2] == anchor_rank - 3 &&
            (board[2][3] == anchor_rank - 3 ||
             board[3][2] == anchor_rank - 3)) ||
           (board[1][3] == anchor_rank - 3 &&
            (board[0][3] == anchor_rank - 3 ||
             board[2][3] == anchor_rank - 3)))) ||
    (board[3][0] == anchor_rank - 1 && board[3][1] == anchor_rank - 2 &&
     board[3][2] == anchor_rank - 3 &&
     (board[2][2] == anchor_rank - 3 || board[3][3] == anchor_rank - 3));
}

#ifdef BIG_TUPLES
using Tuple11 = Tuple<1, 1, 1, 9,   // row 0
                      1, 1,10, 9,   // row 1
                      9, 9, 9, 9,   // row 2
                      8, 8, 8, 8>;  // row 3

template <>
const int Tuple11::kMaxAnchorRank = 9;
template <>
const char Tuple11::kTupleFile[] = "tuple_moves.11b";
#else
using Tuple11 = Tuple<1, 1, 1, 7,   // row 0
                      1, 1, 8, 7,   // row 1
                      7, 7, 7, 7,   // row 2
                      6, 6, 6, 6>;  // row 3

template <>
const int Tuple11::kMaxAnchorRank = 7;
template <>
const char Tuple11::kTupleFile[] = "tuple_moves.11a";
#endif

#endif

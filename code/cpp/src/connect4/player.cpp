#include <connect4/player.h>

using namespace connect4;


// Refer to https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
int8_t Player::_negamaxPlayer(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardMap& memo) {
    auto it = memo.find(board);
    if (it != memo.end()) {
        return it->second;
    }

    if (board.opponentWins()) {
        memo[board] = -depth;
        return -depth;
    } else if (depth == 0 || board.isDraw()) {
        memo[board] = 0;
        return 0;
    }

    int8_t originalAlpha = alpha;

    int8_t score = -42;
    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            continue;
        }

        Board newBoard = board;
        newBoard.placePlayer(col);

        score = std::max<int8_t>(score, -_negamaxOpponent(newBoard, depth - 1, -beta, -alpha, memo));
        alpha = std::max<int8_t>(alpha, score);
        if (alpha >= beta) {
            break;
        } 
    }

    memo[board] = score;
    return score;
}


int8_t Player::_negamaxOpponent(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardMap& memo) {
    auto it = memo.find(board);
    if (it != memo.end()) {
        return it->second;
    }

    if (board.playerWins()) {
        memo[board] = -depth;
        return -depth;
    } else if (depth == 0 || board.isDraw()) {
        memo[board] = 0;
        return 0;
    }

    int8_t originalAlpha = alpha;

    int8_t score = -42;
    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            continue;
        }

        Board newBoard = board;
        newBoard.placeOpponent(col);

        score = std::max<int8_t>(score, -_negamaxPlayer(newBoard, depth - 1, -beta, -alpha, memo));
        alpha = std::max<int8_t>(alpha, score);
        if (alpha >= beta) {
            break;
        } 
    }

    memo[board] = score;
    return score;
}


void Player::_getScores(const Board& board, ScoreArray& scores) {
    BoardMap memo;

    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            scores[col] = -42;
            continue;
        }

        Board newBoard = board;
        newBoard.placePlayer(col);
        scores[col] = -_negamaxOpponent(newBoard, _maxDepth - 1, -42, 42, memo);
    }
}
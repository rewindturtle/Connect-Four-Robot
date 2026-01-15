#include <connect4/player.h>

using namespace connect4;


// Refer to https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
int8_t Player::_negamaxPlayer(const Board& board, uint8_t depth, int8_t alpha, int8_t beta) {
    int8_t originalAlpha = alpha;

    int8_t score = -42;
    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            continue;
        }

        Board newBoard = board;
        newBoard.placePlayer(col);

        auto it = _memo.find(newBoard);
        if (it == _memo.end()) {
            it = _memo.emplace(newBoard, BoardInfo{}).first;
        } else {
            // If two states are identical, they have the same amount of turns played
            // So we don't need to check depth here
            switch (it->second.flag) {
                case BoardInfo::EXACT:
                    return it->second.score;
                case BoardInfo::LOWERBOUND:
                    if (it->second.score >= beta) {
                        return it->second.score;
                    }
                    break;
                case BoardInfo::UPPERBOUND:
                    if (it->second.score <= originalAlpha) {
                        return it->second.score;
                    }
                    break;
                default:
                    break;
            }
        }

        if (newBoard.playerWins()) {
            score = 42 - static_cast<int8_t>(_currentTurn + depth);
            it->second.score = score;
            it->second.flag = BoardInfo::EXACT;
            return score;
        } else if (board.isDraw()) {
            it->second.score = 0;
            it->second.flag = BoardInfo::EXACT;
            return 0;
        } else if (depth == _maxDepth) {
            continue;
        }

        
    }

    auto it = _memo.find(board);
    if (it == _memo.end()) {
        it = _memo.emplace(board, BoardInfo{}).first;
    } else {
        // If two states are identical, they have the same amount of turns played
        // So we don't need to check depth here
        if (it->second.flag == BoardInfo::EXACT) {
            return it->second.score;
        } else if (it->second.flag == BoardInfo::LOWERBOUND && it->second.score >= beta) {
            return it->second.score;
        } else if (it->second.flag == BoardInfo::UPPERBOUND && it->second.score <= originalAlpha) {
            return it->second.score;
        }
    }

    int8_t score = -42;
    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            continue;
        }

        Board newBoard = board;
        newBoard.placePlayer(col);

        if (newBoard.playerWins()) {
            score = 42 - static_cast<int8_t>(_currentTurn + depth);
            it->second.score = score;
            it->second.flag = BoardInfo::EXACT;
            return score;
        } else if (board.isDraw()) {
            it->second.score = 0;
            it->second.flag = BoardInfo::EXACT;
        } else if (depth == _maxDepth) {
            continue;
        }

        score = std::max<int8_t>(score, -_negamaxOpponent(newBoard, depth + 1, -alpha, -beta));
        alpha = std::max<int8_t>(alpha, score);
        if (alpha >= beta) {
            break;
        } 
    }

    it->second.score = score;
    if (score <= originalAlpha) {
        it->second.flag = BoardInfo::UPPERBOUND;
    } else if (score >= beta) {
        it->second.flag = BoardInfo::LOWERBOUND;
    } else {
        it->second.flag = BoardInfo::EXACT;
    }

    return score;
}


int8_t Player::_negamaxOpponent(const Board& board, uint8_t depth, int8_t alpha, int8_t beta) {
    if (board.playerWins()) {
        return static_cast<int8_t>(_currentTurn + depth) - 42;
    } else if (depth == _maxDepth || board.isDraw()) {
        return 0;
    }

    int8_t originalAlpha = alpha;

    auto it = memo.find(board);
    if (it == memo.end()) {
        int8_t score = -42;
        for (uint8_t col = 0; col < 7; ++col) {
            if (board.isColumnFull(col)) {
                continue;
            }

            Board newBoard = board;
            newBoard.placeOpponent(col);
            score = std::max<int8_t>(score, -_negamaxPlayer(newBoard, depth + 1, -alpha, -beta));
            alpha = std::max<int8_t>(alpha, score);
            if (alpha >= beta) {
                break;
            }
        }

        BoardInfo& info = memo[board];
        info.score = score;
        info.depth = depth;
        if (score <= originalAlpha) {
            info.flag = BoardInfo::UPPERBOUND;
        } else if (score >= beta) {
            info.flag = BoardInfo::LOWERBOUND;
        } else {
            info.flag = BoardInfo::EXACT;
        }
        return score;
    } else {
        if (it->second.depth <= depth) {
            if (it->second.flag == BoardInfo::EXACT) {
                return it->second.score;
            } else if (it->second.flag == BoardInfo::LOWERBOUND && it->second.score >= beta) {
                return it->second.score;
            } else if (it->second.flag == BoardInfo::UPPERBOUND && it->second.score <= originalAlpha) {
                return it->second.score;
            }
        }

        int8_t score = -42;
        for (uint8_t col = 0; col < 7; ++col) {
            if (board.isColumnFull(col)) {
                continue;
            }

            Board newBoard = board;
            newBoard.placeOpponent(col);
            score = std::max<int8_t>(score, -_negamaxPlayer(newBoard, depth + 1, -alpha, -beta));
            alpha = std::max<int8_t>(alpha, score);
            if (alpha >= beta) {
                break;
            }
        }

        it->second.score = score;
        it->second.depth = depth;
        if (score <= originalAlpha) {
            it->second.flag = BoardInfo::UPPERBOUND;
        } else if (score >= beta) {
            it->second.flag = BoardInfo::LOWERBOUND;
        } else {
            it->second.flag = BoardInfo::EXACT;
        }

        return score;
    }
}


std::array<int8_t, 7> Player::_getMoveScores(const Board& board) {
    std::array<int8_t, 7> moveScores = {};
    BoardMap memo;

    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            moveScores[col] = -42;
            continue;
        }

        Board newBoard = board;
        newBoard.placePlayer(col);
        moveScores[col] = -_negamaxOpponent(newBoard, 1, -42, 42, memo);
    }

    return moveScores;
}

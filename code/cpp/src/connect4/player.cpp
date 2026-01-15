#include <connect4/player.h>

#include <chrono>

using namespace connect4;


Player::Player() : _board() {
    _timerThread = std::thread(&Player::_timerThreadFunc, this);
}


Player::~Player() {
    _endThreads = true;
    _timerCV.notify_one();
    if (_timerThread.joinable()) _timerThread.join();
}


// Refer to https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
Player::SearchResult Player::_negamaxPlayer(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardSet& visited) {
    if (_isTimeOut) {
        // Searched time exceeded, return neutral score
        return SearchResult{0, false};
    }

    auto it = _memo.find(board);
    if (it != _memo.end()) {
        // Board state's final score already previously computed
        return SearchResult{it->second, true};
    }

    if (visited.find(board) != visited.end()) {
        // Board state was already evaluated in the current search path
        return SearchResult{0, false};
    }

    if (board.opponentWins()) {
        int8_t score = _getScore(depth);
        _memo[board] = score;
        return SearchResult{score, true};
    }
    
    if (board.isDraw()) {
        _memo[board] = 0;
        return SearchResult{0, true};
    } 
    
    if (depth == _maxDepth) {
        return SearchResult{0, false};
    }

    SearchResult result{MIN_SCORE, false};
    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            continue;
        }

        Board newBoard = board;
        newBoard.placePlayer(col);

        SearchResult candidate = _negamaxOpponent(newBoard, depth + 1, -beta, -alpha, visited);

        if (_isTimeOut) {
            return SearchResult{0, false};
        }

        if (-candidate.score > result.score) {
            result.score = -candidate.score;
            result.exact = candidate.exact;

            alpha = std::max<int8_t>(alpha, result.score);
            if (alpha >= beta) {
                break;
            }
        }
    }

    if (result.exact) {
        _memo[board] = result.score;
    } else {
        visited.insert(board);
    }

    return result;
}


Player::SearchResult Player::_negamaxOpponent(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardSet& visited) {
    if (_isTimeOut) {
        // Searched time exceeded, return neutral score
        return SearchResult{0, false};
    }

    auto it = _memo.find(board);
    if (it != _memo.end()) {
        // Board state's final score already previously computed
        return SearchResult{it->second, true};
    }

    if (visited.find(board) != visited.end()) {
        // Board state was already evaluated in the current search path
        return SearchResult{0, false};
    }

    if (board.playerWins()) {
        int8_t score = _getScore(depth);
        _memo[board] = score;
        return SearchResult{score, true};
    }
    
    if (board.isDraw()) {
        _memo[board] = 0;
        return SearchResult{0, true};
    } 
    
    if (depth == _maxDepth) {
        return SearchResult{0, false};
    }

    SearchResult result{MIN_SCORE, false};
    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            continue;
        }

        Board newBoard = board;
        newBoard.placeOpponent(col);

        SearchResult candidate = _negamaxPlayer(newBoard, depth + 1, -beta, -alpha, visited);

        if (_isTimeOut) {
            return SearchResult{0, false};
        }

        if (-candidate.score > result.score) {
            result.score = -candidate.score;
            result.exact = candidate.exact;

            alpha = std::max<int8_t>(alpha, result.score);
            if (alpha >= beta) {
                break;
            }
        }
    }

    if (result.exact) {
        _memo[board] = result.score;
    } else {
        visited.insert(board);
    }

    return result;
}


// Runs a timer thread in the background that sets the timeout flag after _timeOutMS milliseconds
void Player::_timerThreadFunc() {
    auto timeOutMS = std::chrono::milliseconds(_timeOutMS);

    std::mutex timerMutex;
    std::unique_lock<std::mutex> lock(timerMutex);
    while (true) {
        _timerCV.wait(lock, [this]() {return _startTimer || _endThreads;});

        if (_endThreads) return;

        _startTimer = false;
        _isTimeOut = false;
        auto startTime = std::chrono::steady_clock::now();
        while (true) {
            if (_endThreads) {
                _isTimeOut = true;
                return;
            }

            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed >= timeOutMS) {
                _isTimeOut = true;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}


void Player::_getScores(ScoreArray& scores) {
    _maxDepth = 4;
    scores.fill(MIN_SCORE);

    _startTimer = true;
    _isTimeOut = false;
    _timerCV.notify_one();

    while (!_isTimeOut) {
        BoardSet visited;

        for (uint8_t col = 0; col < 7; ++col) {
            if (_board.isColumnFull(col)) {
                scores[col] = MIN_SCORE;
                continue;
            }

            Board newBoard = _board;
            newBoard.placePlayer(col);
            SearchResult result = _negamaxOpponent(newBoard, 1, MIN_SCORE, MAX_SCORE, visited);

            if (_isTimeOut) {
                return;
            }

            scores[col] = -result.score;
        }

        _maxDepth++;
        if (_maxDepth > _globalMaxDepth) {
            return;
        }
    }
}


uint8_t Player::chooseMove() {
    ScoreArray scores;
    _getScores(scores);

    int8_t bestScore = MIN_SCORE;
    uint8_t bestScoreCount = 0;
    for (uint8_t col = 0; col < 7; ++col) {
        const int8_t score = scores[col];
        if (score > bestScore) {
            bestScore = score;
            bestScoreCount = 1;
        } else if (score == bestScore) {
            bestScoreCount++;
        }
    }

    uint8_t chosenIdx;
    if (bestScoreCount == 1) {
        chosenIdx = 0;
    } else if (bestScoreCount != 0) {
        std::uniform_int_distribution<int> dist(0, static_cast<int>(bestScoreCount) - 1);
        chosenIdx = static_cast<uint8_t>(dist(_rng));
    } else {
        return 0;
    }

    uint8_t count = 0;
    for (uint8_t col = 0; col < 7; ++col) {
        if (scores[col] == bestScore) {
            if (count == chosenIdx) {
                return col;
            }
            ++count;
        }
    }

    return 0;
}


void Player::reset(bool hardReset) {
    _board.reset();
    _currentTurn = 0;
    _isTimeOut = false;
    _startTimer = false;
    if (hardReset) {
        _memo.clear();
    }
}


void Player::applyPlayerMove() {
    uint8_t col = chooseMove();
    _board.placePlayer(col);
    _currentTurn++;
}


void Player::applyOpponentMove(uint8_t col) {
    _board.placeOpponent(col);
    _currentTurn++;
}

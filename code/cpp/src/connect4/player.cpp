#include <connect4/player.h>

#include <chrono>

using namespace connect4;


Player::Player() : _board() {}


Player::~Player() {
    _endThreads = true;

    _timerCV.notify_one();
    _idleSearchCV.notify_one();
    _gameCV.notify_one();
    
    if (_timerThread.joinable()) _timerThread.join();
    if (_idleSearchThread.joinable()) _idleSearchThread.join();
    if (_gameThread.joinable()) _gameThread.join();
}


// Refer to https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
int8_t Player::_negamaxPlayer(const Board& board, uint8_t depth, int8_t alpha, int8_t beta) {
    if (_isTimeOut) {
        // Searched time exceeded, return neutral score
        return 0;
    }

    {
        std::lock_guard<std::mutex> lock(_memoMutex);

        auto it = _memo.find(board);
        if (it == _memo.end()) {
            it = _memo.emplace(board, SearchResult{0, NOT_SET}).first;
        } else {
            switch (it->second.flag) {
                case EXACT:
                    return it->second.score;
                case LOWERBOUND:
                    if (it->second.score >= beta) {
                        return it->second.score;
                    }
                    break;
                case UPPERBOUND:
                    if (it->second.score <= alpha) {
                        return it->second.score;
                    }
                    break;
                default:
                    break;
            }
        }

        if (board.opponentWins()) {
            int8_t score = _getScore(depth);
            it->second.score = score;
            it->second.flag = EXACT;
            return score;
        }
        
        if (board.isDraw()) {
            it->second.score = 0;
            it->second.flag = EXACT;
            return 0;
        } 
    }
    
    if (depth == _maxDepth) {
        return 0;
    }

    int8_t originalAlpha = alpha;
    int8_t maxScore = MIN_SCORE;
    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            continue;
        }

        Board newBoard = board;
        newBoard.placePlayer(col);

        int8_t score = -_negamaxOpponent(newBoard, depth + 1, -beta, -alpha);
        if (_isTimeOut) {
            return 0;
        }

        if (score > maxScore) {
            maxScore = score;
            if (maxScore > alpha) {
                alpha = maxScore;
                if (alpha >= beta) {
                    break;
                }
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(_memoMutex);
        auto& entry = _memo[board];
        entry.score = maxScore;
        if (maxScore <= originalAlpha) {
            entry.flag = UPPERBOUND;
        } else if (maxScore >= beta) {
            entry.flag = LOWERBOUND;
        } else {
            entry.flag = EXACT;
        }
    }

    return maxScore;
}


int8_t Player::_negamaxOpponent(const Board& board, uint8_t depth, int8_t alpha, int8_t beta) {
    if (_isTimeOut) {
        // Searched time exceeded, return neutral score
        return 0;
    }

    {
        std::lock_guard<std::mutex> lock(_memoMutex);

        auto it = _memo.find(board);
        if (it == _memo.end()) {
            it = _memo.emplace(board, SearchResult{0, NOT_SET}).first;
        } else {
            switch (it->second.flag) {
                case EXACT:
                    return it->second.score;
                case LOWERBOUND:
                    if (it->second.score >= beta) {
                        return it->second.score;
                    }
                    break;
                case UPPERBOUND:
                    if (it->second.score <= alpha) {
                        return it->second.score;
                    }
                    break;
                default:
                    break;
            }
        }

        if (board.playerWins()) {
            int8_t score = _getScore(depth);
            it->second.score = score;
            it->second.flag = EXACT;
            return score;
        }
        
        if (board.isDraw()) {
            it->second.score = 0;
            it->second.flag = EXACT;
            return 0;
        } 
    }
    
    if (depth == _maxDepth) {
        return 0;
    }

    int8_t originalAlpha = alpha;
    int8_t maxScore = MIN_SCORE;
    for (uint8_t col = 0; col < 7; ++col) {
        if (board.isColumnFull(col)) {
            continue;
        }

        Board newBoard = board;
        newBoard.placeOpponent(col);

        int8_t score = -_negamaxPlayer(newBoard, depth + 1, -beta, -alpha);
        if (_isTimeOut) {
            return 0;
        }

        if (score > maxScore) {
            maxScore = score;
            if (maxScore > alpha) {
                alpha = maxScore;
                if (alpha >= beta) {
                    break;
                }
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(_memoMutex);
        auto& entry = _memo[board];
        entry.score = maxScore;
        if (maxScore <= originalAlpha) {
            entry.flag = UPPERBOUND;
        } else if (maxScore >= beta) {
            entry.flag = LOWERBOUND;
        } else {
            entry.flag = EXACT;
        }
    }

    return maxScore;
}


// Runs a timer thread in the background that sets the timeout flag after _timeOutMS milliseconds
void Player::_timerThreadFunc() {
    std::unique_lock<std::mutex> lock(_timerMutex);

    while (true) {
        _timerCV.wait(lock, [this]() {return _runTimer || _endThreads;});
        if (_endThreads) {
            _isTimeOut = true;
            return;
        }
        
        auto startTime = std::chrono::steady_clock::now();
        while (true) {
            if (_endThreads) {
                _isTimeOut = true;
                return;
            }

            uint32_t thinkingTime = _updateThinkingTimeMS(startTime);
            if (!_runTimer || thinkingTime >= _maxThinkingTime) {
                _isTimeOut = true;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        _runTimer = false;
    }
}


void Player::_idleSearchThreadFunc() {
    std::unique_lock<std::mutex> lock(_idleSearchMutex);
    _isIdleSearching = false;

    while (true) {      
        _idleSearchCV.wait(lock, [this]() {return !_pauseIdleSearch || _endThreads;});
        
        if (_endThreads) return;
        _pauseIdleSearch = false;
        _isIdleSearching = true;

        _maxDepth = 4;
        while (!_pauseIdleSearch) {
            uint8_t numExact = 0;
            for (uint8_t col = 0; col < 7; ++col) {
                if (_endThreads) return;
                if (_pauseIdleSearch) break;

                if (_board.isColumnFull(col)) {
                    numExact++;
                    continue;
                }

                Board newBoard = _board;
                newBoard.placePlayer(col);

                {
                    std::lock_guard<std::mutex> lock(_memoMutex);
                    auto it = _memo.find(newBoard);
                    if (it != _memo.end() && it->second.flag == EXACT) {
                        numExact++;
                        continue;
                    }
                }

                _negamaxOpponent(newBoard, 1, MIN_SCORE, MAX_SCORE);
            }

            // If all seven columns have exact scores, there is no need to continue searching deeper
            if (numExact == 7) {
                _pauseIdleSearch = true;
                break;
            }

            _maxDepth++;
        }

        _isIdleSearching = false;
    }
}


void Player::_getScores(ScoreArray& scores) {
    _maxDepth = std::min<uint8_t>(4, _globalMaxDepth);
    scores.fill(MIN_SCORE);
    if (!_allowIdleSearch) {
        _memoMutex.lock();
        _memo.clear();
        _memoMutex.unlock();
    }

    // Reset the timer
    {
        _runTimer = false;

        // The thread will block here until the timer is fully reset
        std::lock_guard<std::mutex> lock(_timerMutex);

        _isTimeOut = false;
        _runTimer = true;
        _timerCV.notify_one();
    }

    while (!_isTimeOut) {
        for (uint8_t col = 0; col < 7; ++col) {
            if (_board.isColumnFull(col)) {
                scores[col] = MIN_SCORE;
                continue;
            }

            Board newBoard = _board;
            newBoard.placePlayer(col);

            int8_t score = -_negamaxOpponent(newBoard, 1, MIN_SCORE, MAX_SCORE);
            if (_isTimeOut) return;

            scores[col] = score;
        }

        _maxDepth++;
        if (_maxDepth > _globalMaxDepth) {
            _runTimer = false;
            return;
        }
    }
}


uint8_t Player::_chooseMove() {
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


void Player::_reset(bool hardReset) {
    _endThreads = true;

    _timerCV.notify_one();
    _idleSearchCV.notify_one();
    _gameCV.notify_one();

    if (_timerThread.joinable()) _timerThread.join();
    if (_idleSearchThread.joinable()) _idleSearchThread.join();
    if (_gameThread.joinable()) _gameThread.join();
    _endThreads = false;

    _board.reset();
    _turnCount = 0;
    _isTimeOut = false;
    _runTimer = false;
    _thinkingTimeMS.store(0, std::memory_order_release);
    _winner.store(NO_WINNER, std::memory_order_release);

    if (hardReset) {
        _memoMutex.lock();
        _memo.clear();
        _memoMutex.unlock();
    }
}


void Player::_applyPlayerMove() {
    uint8_t col = _chooseMove();
    _board.placePlayer(col);
    _turnCount++;
}


void Player::setDifficulty(Difficulty difficulty) {
    if (_isPlaying) return;

    _playerDifficulty = difficulty;
}



void Player::_applyDifficultySettings() {
    switch (_playerDifficulty) {
        case DIFFICULTY_0:
            _globalMaxDepth = 2;
            _maxThinkingTime = 1000;
            _allowIdleSearch = false;
            break;
        case DIFFICULTY_1:
            _globalMaxDepth = 3;
            _maxThinkingTime = 1500;
            _allowIdleSearch = false;
            break;
        case DIFFICULTY_2:
            _globalMaxDepth = 4;
            _maxThinkingTime = 2500;
            _allowIdleSearch = false;
            break;
        case DIFFICULTY_3:
            _globalMaxDepth = 5;
            _maxThinkingTime = 4000;
            _allowIdleSearch = false;
            break;
        case DIFFICULTY_4:
            _globalMaxDepth = 5;
            _maxThinkingTime = 4000;
            _allowIdleSearch = true;
            break;
        case DIFFICULTY_5:
            _globalMaxDepth = 6;
            _maxThinkingTime = 5000;
            _allowIdleSearch = true;
            break;
        case DIFFICULTY_6:
            _globalMaxDepth = 7;
            _maxThinkingTime = 7000;
            _allowIdleSearch = true;
            break;
        case DIFFICULTY_7:
            _globalMaxDepth = 8;
            _maxThinkingTime = 10000;
            _allowIdleSearch = true;
            break;
        case DIFFICULTY_8:
            _globalMaxDepth = 9;
            _maxThinkingTime = 15000;
            _allowIdleSearch = true;
            break;
        default:
            _globalMaxDepth = 4;
            _maxThinkingTime = 5000;
            _allowIdleSearch = true;
            break;
    }
}


void Player::start(bool playerMovesFirst) {
    if (_isPlaying) {
        return;
    }
    _isPlaying = true;

    _reset(true);
    _applyDifficultySettings();

    _timerThread = std::thread(&Player::_timerThreadFunc, this);
    if (_allowIdleSearch) {
        _idleSearchThread = std::thread(&Player::_idleSearchThreadFunc, this);
    }

    _isPlayerTurn = playerMovesFirst;
    _gameThread = std::thread(&Player::_play, this);
}


void Player::_play() {
    std::mutex gameMutex;
    std::unique_lock<std::mutex> lock(gameMutex);

    while (!_endThreads) {
        _gameCV.wait(lock, [this]() {return _isPlayerTurn || _endThreads;});

        if (_endThreads) break;

        // Ensure the idle search thread is paused while the player is making a move
        _pauseIdleSearch = true;
        std::lock_guard<std::mutex> idleLock(_idleSearchMutex);

        // Check if game is over (opponent won or draw)
        if (_board.opponentWins()) {
            _winner.store(OPPONENT_WINS, std::memory_order_release);
            break;
        } else if (_board.isDraw()) {
            _winner.store(DRAW, std::memory_order_release);
            break;
        }

        _applyPlayerMove();

        // Check if game is over (player won or draw)
        if (_board.playerWins()) {
            _winner.store(PLAYER_WINS, std::memory_order_release);
            break;
        } else if (_board.isDraw()) {
            _winner.store(DRAW, std::memory_order_release);
            break;
        }

        _isPlayerTurn = false;
    }

    _isPlaying = false;
}



bool Player::applyOpponentMove(uint8_t col) {
    // Can't apply opponent move if the game is not active or it's currently the player's turn
    if (!_isPlaying || _isPlayerTurn) {
        return false;
    }

    // Ensure the idle search thread is paused while the opponent is making a move
    _pauseIdleSearch = true;
    std::lock_guard<std::mutex> idleLock(_idleSearchMutex);

    // Can't apply move to a full column
    if (_board.isColumnFull(col)) {
        return false;
    }

    _board.placeOpponent(col);
    _turnCount++;
    _isPlayerTurn = true;
    _gameCV.notify_one();

    return true;
}


size_t Player::getMemoSize() const {
    std::lock_guard<std::mutex> lock(_memoMutex);
    return _memo.size();
}

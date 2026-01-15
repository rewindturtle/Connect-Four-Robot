#pragma once

#include <connect4/board.h>
#include <utils/atomic_flag.h>

#include <unordered_map>
#include <unordered_set>
#include <array>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <random>

#define MIN_SCORE -42
#define MAX_SCORE 42


namespace connect4 {
    class Player {
        enum SearchFlag : uint8_t {
            NOT_SET = 0,
            EXACT = 1,
            LOWERBOUND = 2,
            UPPERBOUND = 3
        };

        struct SearchResult {
            int8_t score = 0;
            SearchFlag flag = NOT_SET;
        };

        using BoardMap = std::unordered_map<Board, SearchResult, BoardHash>;
        using ScoreArray = std::array<int8_t, 7>;

        public:
            enum Turn : uint8_t {
                PLAYER = 0,
                OPPONENT = 1
            };

            enum Difficulty : uint8_t {
                DIFFICULTY_0 = 0,
                DIFFICULTY_1 = 1,
                DIFFICULTY_2 = 2,
                DIFFICULTY_3 = 3,
                DIFFICULTY_4 = 4,
                DIFFICULTY_5 = 5,
                DIFFICULTY_6 = 6,
                DIFFICULTY_7 = 7,
                DIFFICULTY_8 = 8
            };

            enum Winner : uint8_t {
                NO_WINNER = 0,
                PLAYER_WINS = 1,
                OPPONENT_WINS = 2,
                DRAW = 3
            };

        private:
            // Game state
            Board _board;
            uint8_t _turnCount = 0;
            std::atomic<Winner> _winner{NO_WINNER};
            utils::AtomicFlag _isPlayerTurn{false};

            // Difficulty parameters
            uint32_t _maxThinkingTime = 5000;
            uint8_t _globalMaxDepth = 8;
            bool _allowIdleSearch = true;

            // Player Info
            std::atomic<uint32_t> _thinkingTimeMS{0};
            utils::AtomicFlag _isPlaying{false};
            Difficulty _playerDifficulty = DIFFICULTY_0;

            // Search variables
            BoardMap _memo;
            uint8_t _maxDepth = 4;

            // Game Thread
            std::thread _gameThread;
            mutable std::condition_variable _gameCV;
            
            // Timer Thread
            mutable std::condition_variable _timerCV;
            mutable std::mutex _timerMutex;
            std::thread _timerThread;
            utils::AtomicFlag _isTimeOut{false};
            utils::AtomicFlag _runTimer{false};

            // Idle Search Thread
            mutable std::condition_variable _idleSearchCV;
            mutable std::mutex _idleSearchMutex;
            std::thread _idleSearchThread;
            utils::AtomicFlag _pauseIdleSearch{false};
            utils::AtomicFlag _isIdleSearching{false};

            // Thread control
            utils::AtomicFlag _endThreads{false};
            mutable std::mutex _memoMutex;
            
            // Misc
            std::mt19937 _rng{std::random_device{}()};
            

            inline uint32_t _updateThinkingTimeMS(std::chrono::steady_clock::time_point startTime) {
                uint32_t thinkingTimeMS = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
                _thinkingTimeMS.store(thinkingTimeMS, std::memory_order_release);
                return thinkingTimeMS;
            }

            inline int8_t _getScore(uint8_t depth) const {
                return MAX_SCORE - static_cast<int8_t>(_turnCount + depth);
            }

            void _timerThreadFunc();
            void _idleSearchThreadFunc();
            void _play();

            int8_t _negamaxPlayer(const Board& board, uint8_t depth, int8_t alpha, int8_t beta);
            int8_t _negamaxOpponent(const Board& board, uint8_t depth, int8_t alpha, int8_t beta);
            void _getScores(ScoreArray& scores);

            void _reset(bool hardReset = false);
            void _applyDifficultySettings();
            uint8_t _chooseMove();
            void _applyPlayerMove();
        public:
            Player();
            ~Player();

            inline Board getBoard() const {return _board;}

            inline uint8_t getMaxDepth() const {return _globalMaxDepth;}
            inline uint8_t getTurnCount() const {return _turnCount;}
            inline uint32_t getThinkingTimeMS() const {return _thinkingTimeMS.load(std::memory_order_acquire);}
            inline bool isPlaying() const {return _isPlaying;}
            inline Turn getCurrentTurn() const {return _isPlayerTurn ? PLAYER : OPPONENT;}
            inline bool isIdleSearching() const {return _isIdleSearching;}
            inline Difficulty getDifficulty() const {return _playerDifficulty;}
            inline Winner getWinner() const {return _winner.load(std::memory_order_acquire);}
            void setDifficulty(Difficulty difficulty);
            size_t getMemoSize() const;
            
            bool applyOpponentMove(uint8_t col);

            void start(bool playerMovesFirst = false);
    };
}

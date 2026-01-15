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
        using BoardMap = std::unordered_map<Board, int8_t, BoardHash>;
        using BoardSet =  std::unordered_set<Board, BoardHash>;
        using ScoreArray = std::array<int8_t, 7>;

        struct SearchResult {
            int8_t score = 0;
            bool exact = false;
        };

        private:
            BoardMap _memo;
            Board _board;
            uint8_t _globalMaxDepth = 8;
            uint8_t _maxDepth = 4;
            uint8_t _currentTurn = 0;
            utils::AtomicFlag _isTimeOut{false};
            utils::AtomicFlag _startTimer{false};
            utils::AtomicFlag _endTimer{false};
            utils::AtomicFlag _endThreads{false};
            mutable std::condition_variable _timerCV;
            uint32_t _timeOutMS = 5000;
            std::atomic<uint32_t> _thinkingTimeMS{0};
            std::thread _timerThread;
            std::mt19937 _rng{std::random_device{}()};
            bool _useMemoization = true;

            inline uint32_t _updateThinkingTimeMS(std::chrono::steady_clock::time_point startTime) {
                uint32_t thinkingTimeMS = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
                _thinkingTimeMS.store(thinkingTimeMS, std::memory_order_release);
                return thinkingTimeMS;
            }

            inline int8_t _getScore(uint8_t depth) const {
                return MAX_SCORE - static_cast<int8_t>(_currentTurn + depth);
            }

            void _timerThreadFunc();

            SearchResult _negamaxPlayer(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardSet& visited);
            SearchResult _negamaxOpponent(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardSet& visited);
            int8_t _negamaxPlayerNoMemo(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardMap& visited);
            int8_t _negamaxOpponentNoMemo(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardMap& visited);
            
            void _getScores(ScoreArray& scores);
            void _search(ScoreArray& scores);
            void _searchNoMemo(ScoreArray& scores);
        public:
            Player();
            ~Player();

            inline Board getBoard() const {return _board;}

            inline uint8_t getMaxDepth() const {return _globalMaxDepth;}
            inline uint32_t getThinkingTimeMS() const {return _thinkingTimeMS.load(std::memory_order_acquire);}
            inline uint8_t getCurrentTurn() const {return _currentTurn;}

            void reset(bool hardReset = false);
            uint8_t chooseMove();

            void applyPlayerMove();
            void applyOpponentMove(uint8_t col);
    };
}

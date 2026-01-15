#pragma once

#include <connect4/board.h>
#include <unordered_map>
#include <array>


namespace connect4 {
    class Player {
        struct BoardInfo {
            enum Flag : uint8_t {
                NOT_SET = 0,
                EXACT = 1,
                LOWERBOUND = 2,
                UPPERBOUND = 3
            };

            int8_t score = 0;
            Flag flag = NOT_SET;

            BoardInfo() : score(0), flag(NOT_SET) {}
        };

        using BoardMap = std::unordered_map<Board, BoardInfo>;

        private:
            BoardMap _memo;
            Board _board;
            uint8_t _currentTurn = 0;
            uint8_t _maxDepth = 7;

            int8_t _negamaxPlayer(const Board& board, uint8_t depth, int8_t alpha, int8_t beta);
            int8_t _negamaxOpponent(const Board& board, uint8_t depth, int8_t alpha, int8_t beta);
            std::array<int8_t, 7> _getMoveScores(const Board& board);
        public:
            Player() : _board() {}

            inline Board getBoard() const {return _board;}

            inline void setMaxDepth(uint8_t depth) {_maxDepth = depth;}
            inline uint8_t getMaxDepth() const {return _maxDepth;}
    };
}

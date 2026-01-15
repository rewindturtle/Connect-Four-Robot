#pragma once

#include <connect4/board.h>
#include <unordered_map>
#include <array>


namespace connect4 {
    class Player {
        using BoardMap = std::unordered_map<Board, int8_t, BoardHash>;
        using ScoreArray = std::array<int8_t, 7>;

        private:
            Board _board;
            uint8_t _maxDepth = 7;
            uint8_t _currentTurn = 0;

            int8_t _negamaxPlayer(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardMap& memo);
            int8_t _negamaxOpponent(const Board& board, uint8_t depth, int8_t alpha, int8_t beta, BoardMap& memo);
            void _getScores(const Board& board, ScoreArray& scores);
        public:
            Player() : _board() {}

            inline Board getBoard() const {return _board;}

            inline void setMaxDepth(uint8_t depth) {_maxDepth = depth;}
            inline uint8_t getMaxDepth() const {return _maxDepth;}
    };
}

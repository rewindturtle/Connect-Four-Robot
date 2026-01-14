#pragma once

#include <cstdint>


namespace connect4 {
    class Board {
        private:
            // Bit boards are stored column-wise from the bottom left
            uint64_t _totalBoard;
            uint64_t _redBoard;
        public:
            Board() : _totalBoard(0), _redBoard(0) {}

            inline uint64_t getTotalBoard() const {return _totalBoard;}
            inline uint64_t getRedBoard() const {return _redBoard;}
            inline uint64_t getYellowBoard() const {return _totalBoard ^ _redBoard;}

            inline bool isFilled(uint8_t index) const {return (_totalBoard >> index) & 0x1ULL;}
            inline bool isRed(uint8_t index) const {return (_redBoard >> index) & 0x1ULL;}
            inline bool isYellow(uint8_t index) const {return (getYellowBoard() >> index) & 0x1ULL;}

            inline void setRed(uint8_t index) {
                _totalBoard |= (0x1ULL << index);
                _redBoard |= (0x1ULL << index);
            }

            inline void setYellow(uint8_t index) {
                _totalBoard |= (0x1ULL << index);
            }

            inline void clear(uint8_t index) {
                _totalBoard &= ~(0x1ULL << index);
                _redBoard &= ~(0x1ULL << index);
            }

            inline void reset() {
                _totalBoard = 0;
                _redBoard = 0;
            }

            inline void placeRed(uint8_t col) {
                uint8_t idx = 6 * col;
                for (uint8_t i = 0; i < 6; ++i) {
                    if (!isFilled(idx)) {
                        setRed(idx);
                        return;
                    }
                    ++idx;
                }
            }

            inline void placeYellow(uint8_t col) {
                uint8_t idx = 6 * col;
                for (uint8_t i = 0; i < 6; ++i) {
                    if (!isFilled(idx)) {
                        setYellow(idx);
                        return;
                    }
                    ++idx;
                }
            }

            inline bool isColumnFull(uint8_t col) const {return isFilled(6 * col + 5);}

            inline bool operator==(const Board& other) const {
                return _totalBoard == other._totalBoard && _redBoard == other._redBoard;
            }

            inline bool operator!=(const Board& other) const {
                return !(*this == other);
            }

            bool redWins() const;
            bool yellowWins() const;

            // 2 ^ 42 - 1 == 4398046511103ULL
            inline bool isDraw() const {return _totalBoard == 4398046511103ULL;}
    };
}

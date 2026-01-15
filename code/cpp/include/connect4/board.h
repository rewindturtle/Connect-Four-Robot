#pragma once

#include <cstdint>


namespace connect4 {
    class Board {
        private:
            // Bit boards are stored column-wise from the bottom left
            uint64_t _totalBoard;
            uint64_t _playerBoard;
        public:
            Board() : _totalBoard(0), _playerBoard(0) {}

            inline uint64_t getTotalBoard() const {return _totalBoard;}
            inline uint64_t getPlayerBoard() const {return _playerBoard;}
            inline uint64_t getOpponentBoard() const {return _totalBoard ^ _playerBoard;}

            inline bool isFilled(uint8_t index) const {return (_totalBoard >> index) & 0x1ULL;}
            inline bool isPlayer(uint8_t index) const {return (_playerBoard >> index) & 0x1ULL;}
            inline bool isOpponent(uint8_t index) const {return (getOpponentBoard() >> index) & 0x1ULL;}

            inline void setPlayer(uint8_t index) {
                _totalBoard |= (0x1ULL << index);
                _playerBoard |= (0x1ULL << index);
            }

            inline void setOpponent(uint8_t index) {
                _totalBoard |= (0x1ULL << index);
            }

            inline void clear(uint8_t index) {
                _totalBoard &= ~(0x1ULL << index);
                _playerBoard &= ~(0x1ULL << index);
            }

            inline void reset() {
                _totalBoard = 0;
                _playerBoard = 0;
            }

            inline void placePlayer(uint8_t col) {
                uint8_t idx = 6 * col;
                for (uint8_t i = 0; i < 6; ++i) {
                    if (!isFilled(idx)) {
                        setPlayer(idx);
                        return;
                    }
                    ++idx;
                }
            }

            inline void placeOpponent(uint8_t col) {
                uint8_t idx = 6 * col;
                for (uint8_t i = 0; i < 6; ++i) {
                    if (!isFilled(idx)) {
                        setOpponent(idx);
                        return;
                    }
                    ++idx;
                }
            }

            inline bool isColumnFull(uint8_t col) const {return isFilled(6 * col + 5);}

            inline bool operator==(const Board& other) const {
                return _totalBoard == other._totalBoard && _playerBoard == other._playerBoard;
            }

            inline bool operator!=(const Board& other) const {
                return !(*this == other);
            }

            bool playerWins() const;
            bool opponentWins() const;

            // 2 ^ 42 - 1 == 4398046511103ULL
            inline bool isDraw() const {return _totalBoard == 4398046511103ULL;}
    };

    struct BoardHash {
        static inline uint64_t splitMix64(uint64_t key) noexcept {
            key += 0x9e3779b97f4a7c15ULL;
            key = (key ^ (key >> 30)) * 0xbf58476d1ce4e5b9ULL;
            key = (key ^ (key >> 27)) * 0x94d049bb133111ebULL;
            return key ^ (key >> 31);
        }

        size_t operator()(const Board& board) const noexcept {
            uint64_t h1 = splitMix64(board.getTotalBoard());
            uint64_t h2 = splitMix64(board.getPlayerBoard());
            return static_cast<size_t>(h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h2 >> 2)));
        }
    };
}

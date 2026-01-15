#include <connect4/board.h>

using namespace connect4;


inline static bool isSet(const uint64_t board, uint8_t index) {
    return (board >> index) & 0x1ULL;
}


static inline bool isWin(const uint64_t board) {
    // Check vertical
    for (uint8_t colIdx = 0; colIdx < 42; colIdx += 6) {
        // For a vertical line, the middle two pieces are always red
        bool isRed2 = isSet(board, colIdx + 2);
        if (!isRed2) {
            continue;
        }

        bool isRed3 = isSet(board, colIdx + 3);
        if (!isRed3) {
            continue;
        }

        bool isRed1 = isSet(board, colIdx + 1);
        if (isRed1) {
            bool isRed0 = isSet(board, colIdx);
            if (isRed0) {
                return true;
            }

            bool isRed4 = isSet(board, colIdx + 4);
            if (isRed4) {
                return true;
            }
        } else {
            bool isRed4 = isSet(board, colIdx + 4);
            if (isRed4) {
                bool isRed5 = isSet(board, colIdx + 5);
                if (isRed5) {
                    return true;
                }
            }
        }
    }

    // Check horizontal
    for (uint8_t rowIdx = 0; rowIdx < 6; ++rowIdx) {
        bool isRed3 = isSet(board, rowIdx + 18);
        
        // For a horizontal line, the middle piece is always red
        if (!isRed3) {
            continue;
        }

        bool isRed2 = isSet(board, rowIdx + 12);
        if (isRed2) {
            bool isRed1 = isSet(board, rowIdx + 6);
            if (isRed1) {
                bool isRed0 = isSet(board, rowIdx);
                if (isRed0) {
                    return true;
                }

                bool isRed4 = isSet(board, rowIdx + 24);
                if (isRed4) {
                    return true;
                }
            } else {
                bool isRed4 = isSet(board, rowIdx + 24);
                if (isRed4) {
                    bool isRed5 = isSet(board, rowIdx + 30);
                    if (isRed5) {
                        return true;
                    }
                }
            }
        } else {
            bool isRed4 = isSet(board, rowIdx + 24);
            if (isRed4) {
                bool isRed5 = isSet(board, rowIdx + 30);
                if (isRed5) {
                    bool isRed6 = isSet(board, rowIdx + 36);
                    if (isRed6) {
                        return true;
                    }
                }
            }
        }
    }

    // Check diagonal (bottom-left to top-right)
    for (uint8_t col = 0; col < 24; col += 6) {
        for (uint8_t row = 0; row < 3; ++row) {
            uint8_t idx = col + row;

            bool isRed00 = isSet(board, idx);
            if (!isRed00) {
                continue;
            }

            bool isRed11 = isSet(board, idx + 7);
            if (!isRed11) {
                continue;
            }

            bool isRed22 = isSet(board, idx + 14);
            if (!isRed22) {
                continue;
            }

            bool isRed33 = isSet(board, idx + 21);
            if (isRed33) {
                return true;
            }
        }
    }

    // Check diagonal (top-left to bottom-right)
    for (uint8_t col = 0; col < 24; col += 6) {
        for (uint8_t row = 0; row < 3; ++row) {
            uint8_t idx = col + row;

            bool isRed00 = isSet(board, idx + 3);
            if (!isRed00) {
                continue;
            }

            bool isRed11 = isSet(board, idx + 8);
            if (!isRed11) {
                continue;
            }

            bool isRed22 = isSet(board, idx + 13);
            if (!isRed22) {
                continue;
            }

            bool isRed33 = isSet(board, idx + 18);
            if (isRed33) {
                return true;
            }
        }
    }

    return false;
}


bool Board::playerWins() const {
    return isWin(_playerBoard);
}


bool Board::opponentWins() const {
    return isWin(getOpponentBoard());
}

#include "board.h"

static const int16_t k_multipliers[BOARD_MAX_BINS] = {1, 3, 5, 10, 5, 3, 1};

void board_layout(Board *board, GSize display) {
  const int16_t w = display.w;
  const int16_t h = display.h;

  board->left = 6;
  board->right = (int16_t)(w - 7);
  board->drop_y = 22;
  board->top = 34;
  board->bottom = (int16_t)(h - 28);
  board->bin_label_y = (int16_t)(h - 22);

  const int16_t play_w = (int16_t)(board->right - board->left);
  const uint8_t rows = (h >= 200) ? 9 : 7;
  const uint8_t pegs_per_even = 6;
  const int16_t row_gap = (int16_t)((board->bottom - board->top) / (rows + 1));
  const int16_t col_gap = (int16_t)(play_w / (pegs_per_even + 1));

  board->peg_count = 0;
  for (uint8_t r = 0; r < rows; r++) {
    const bool odd = (r % 2) != 0;
    const uint8_t cols = odd ? (uint8_t)(pegs_per_even - 1) : pegs_per_even;
    const int16_t y = (int16_t)(board->top + (r + 1) * row_gap);
    const int16_t x_off = odd ? (int16_t)(col_gap / 2) : 0;
    for (uint8_t c = 0; c < cols; c++) {
      if (board->peg_count >= BOARD_MAX_PEGS) break;
      Peg *p = &board->pegs[board->peg_count++];
      p->x = (int16_t)(board->left + x_off + (c + 1) * col_gap);
      p->y = y;
    }
  }

  board->bin_count = BOARD_MAX_BINS;
  const int16_t bin_w = (int16_t)(play_w / BOARD_MAX_BINS);
  for (uint8_t i = 0; i < BOARD_MAX_BINS; i++) {
    board->bins[i].x0 = (int16_t)(board->left + i * bin_w);
    board->bins[i].x1 = (i == BOARD_MAX_BINS - 1)
                            ? board->right
                            : (int16_t)(board->left + (i + 1) * bin_w);
    board->bins[i].multiplier = k_multipliers[i];
  }
}

int8_t board_bin_at(const Board *board, int16_t x) {
  for (uint8_t i = 0; i < board->bin_count; i++) {
    if (x >= board->bins[i].x0 && x < board->bins[i].x1) {
      return (int8_t)i;
    }
  }
  // Clamp to nearest edge bin
  if (x < board->left) return 0;
  return (int8_t)(board->bin_count - 1);
}

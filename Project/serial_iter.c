
int solve_iter(sudoku) {
  uint8_t *plays = NULL;
  int ptr, nplays = N * N;

  /* Array to keep history of previous plays (for backtracking) */
  /* One for each empty square of the initial sudoku */
  plays = (uint8_t*) malloc(nplays*sizeof(uint8_t));

  ptr = 0;
  plays[0] = 1; /* Next play to try. */
  while (1) {
    /* Check if this node has any more branch. */
    if (plays[ptr] > N) {
      if (ptr == 0) {
        /* No solution */

        break;
      } else {
        /* Backtrack */
        plays[--ptr]++;
        unchange(sudoku, ptr, plays[ptr]);

        continue;
      }
    }

    /* Test if this play is invalid */
    if (!valid) {
      /* Branch right */
      plays[ptr]++;
    } else {
      /* Apply change */
      change(sudoku, ptr, plays[ptr]);

      /* Branch down */
      if (++ptr < nplays) {
        plays[ptr] = 1;
      } else {
        /* Puzzle solved */

        break;
      }
    }
  }
}

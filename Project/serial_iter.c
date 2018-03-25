
int solve_iter() {
  uint8_t *plays = NULL;
  int ptr, nplays = to_solve->n * to_solve->n;

  /* Array to keep history of previous plays (for backtracking) */
  plays = (uint8_t*) malloc(nplays*sizeof(uint8_t));

  ptr = 0;
  plays[0] = 1;
  while (ptr < nplays) {
    /* Automatically branch down if square has initial number. */
    if (hasnumber) {
      /* Branch down */
      if (++ptr < nplays) {
        plays[ptr] = 1;
      } else {
        break;
      }
    }

    /* Check if this node has any more branch. */
    if (plays[ptr] > to_solve->n) {
      if (ptr == 0) {
        /* Finished tree */
        break;
      }

      /* Backtrack */
      plays[--ptr]++;
      continue;
    }

    /* Test if this play is valid */
    if (isvalid) {
      /* Branch right */
      plays[ptr]++;
    } else {
      /* Branch down */
      if (++ptr < nplays) {
        plays[ptr] = 1;
      } else {
        break;
      }
    }
  }
}

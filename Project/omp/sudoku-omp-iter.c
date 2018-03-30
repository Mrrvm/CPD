int solve() {
    int ptr_public;

    /*************
          Divide n_plays into public and private
          0...n_shared-1 -> public
          n_shared...n_plays -> privare

          Add ability pto define n_shared and test various values
    *************/

    /*** Create public sudoku map ***/

    /***  Public plays array init (size n_shared) ***/
    uint8_t *plays = to_solve->plays;

    /* Next play to try. */
    plays_public[0] = 1;
    ptr_public = 0;

    /*** Launch threads (parallel) ***/

    /*** Allocate private plays array (size is n_plays - n_shared) ***/

    /*** Allocate private sudoku (even the public part is replicate
         because it can change externally) ***/

    int ptr_private;

    while (1) {
      /*** Public part (critical region) ***/
      while (1) {
        while (1) {
            /*** Iterative algorithm as if plays is from 0 to n_shared-1 ***/
            /*** Using public sudoku map and public_ptr ***/

            /*** If no more public plays exit this and set "ended = 1" ***/
        }

        /*** Make private copy of map. Just iterate through public plays
             and change those in private map without checking validity
             (Other option is copying public plays and making this in private)
        ***/
      }
      /*** End public part (public_ptr == n_shared-1) ***/

      if (ended = 1) break;

      ptr_private = n_shared;

      while (1) {
        /*** Iterative algorithm as if plays is from n_shared to n_plays ***/
        /*** Using private sudoku map ***/

        /*** If reached end, warn all threads and exit ***/
        /*** If not, just continue while ***/
      }
      /** If reached here, no solution was found and ptr_private == n_shared ***/

    }

    /*** Join threads (barrier) ***/

    /*** End parallel ***/
}

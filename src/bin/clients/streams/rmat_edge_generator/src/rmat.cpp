#include <stdlib.h>
#include "rmat_edge_generator.h"

/* Recursively divide a grid of N x N by four to a single point, (i, j).
   Choose between the four quadrants with probability a, b, c, and d.
   Create an edge between node i and j.
 */
void
RMAT (int64_t scale, double a, double b, double c, double d, int64_t *start, int64_t *end)
{
  double norm;

  int64_t bit = 1 << (scale - 1);         /* size of original quandrant     */

  double rn_dbl = (double) rand() / (double) RAND_MAX;

  if      (rn_dbl <= a)         { *start = 0;   *end = 0;   }   /* Q1 */
  else if (rn_dbl <= a + b)     { *start = 0;   *end = bit; }   /* Q2 */
  else if (rn_dbl <= a + b + c) { *start = bit; *end = 0;   }   /* Q3 */
  else                          { *start = bit; *end = bit; }   /* Q4 */

  double rn_dbl_0 = (double) rand() / (double) RAND_MAX;
  double rn_dbl_1 = (double) rand() / (double) RAND_MAX;
  double rn_dbl_2 = (double) rand() / (double) RAND_MAX;
  double rn_dbl_3 = (double) rand() / (double) RAND_MAX;
  double rn_dbl_4 = (double) rand() / (double) RAND_MAX;

  /* Divide grid by 4 by moving bit 1 place to the right */
  for (bit >>= 1; bit > 0; bit >>= 1) {

    /* New probability = old probability +/- at most 10% */
    a *= 0.95 + 0.1 * rn_dbl_0;
    b *= 0.95 + 0.1 * rn_dbl_1;
    c *= 0.95 + 0.1 * rn_dbl_2;
    d *= 0.95 + 0.1 * rn_dbl_3;

    norm = 1.0 / (a + b + c + d);
    a *= norm; b *= norm; c *= norm; d *= norm;

    if      (rn_dbl_4 <= a)         {                             }  /* Q1 */
    else if (rn_dbl_4 <= a + b)     { *end   += bit;              }  /* Q2 */
    else if (rn_dbl_4 <= a + b + c) { *start += bit;              }  /* Q3 */
    else                            { *start += bit; *end += bit; }  /* Q4 */
  }
}

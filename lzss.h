#define EI 11  /* typically 10..13 */
#define EJ  4  /* typically 4..5   */
#define P   1  /* If match length <= P then output one character */
#define N (1 << EI)        /* buffer size           (2^11    = 2048) */
#define F ((1 << EJ) + P)  /* lookahead buffer size (2^4 + 1 = 17)   */
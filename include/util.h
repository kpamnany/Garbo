/*  garbo

    global array toolbox and distributed dynamic scheduler
*/

#ifndef _UTIL_H
#define _UTIL_H

/*  test-and-test-and-set lock macros
 */
#define lock_init(l)                    \
    (l) = 0

#define lock_acquire(l)                 \
    do {                                \
        while ((l))                     \
            cpupause();                 \
    } while (__sync_lock_test_and_set(&(l), 1))
#define lock_release(l)                 \
    (l) = 0


/*  thread yield and delay
 */
#if (__MIC__)
# define cpupause()     _mm_delay_64(100)
//# define waitcycles(c)  _mm_delay_64((c))
# define waitcycles(c) {                \
      uint64_t s=_rdtsc();              \
      while ((_rdtsc()-s)<(c))          \
          _mm_delay_64(100);            \
  }
#else
# define cpupause()     _mm_pause()
# define waitcycles(c) {                \
      uint64_t s=_rdtsc();              \
      while ((_rdtsc()-s)<(c))          \
          _mm_pause();                  \
  }
#endif

#endif /* _UTIL_H */


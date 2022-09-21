#include "common/utils/utils.h"

#include <stdlib.h>
#include <sys/time.h>

namespace resdb {

#define CPU_FREQ 2.2
#define TIME_ENABLE true

uint64_t get_server_clock() {
#if defined(__i386__)
  uint64_t ret;
  __asm__ __volatile__("rdtsc" : "=A"(ret));
#elif defined(__x86_64__)
  unsigned hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  uint64_t ret = ((uint64_t)lo) | (((uint64_t)hi) << 32);
  ret = (uint64_t)((double)ret / CPU_FREQ);
#else
  timespec *tp = new timespec;
  clock_gettime(CLOCK_REALTIME, tp);
  uint64_t ret = tp->tv_sec * 1000000000 + tp->tv_nsec;
  delete tp;
#endif
  return ret;
}

uint64_t get_sys_clock() {
  if (TIME_ENABLE) return get_server_clock();
  return 0;
}

uint64_t get_current_time() {
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  return static_cast<uint64_t>(tv.tv_sec) * 1000000000ll + tv.tv_usec;
}

}  // namespace resdb

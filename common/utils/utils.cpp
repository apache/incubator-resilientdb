/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

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

uint64_t GetCurrentTime() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (uint64_t)(tv.tv_sec) * 1000000 + tv.tv_usec;
}

}  // namespace resdb

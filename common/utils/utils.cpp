#include "common/utils/utils.h"

#include <stdlib.h>
#include <sys/time.h>

namespace resdb {

uint64_t get_server_clock() {
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  return tp->tv_sec * 1000000000 + tp->tv_usec*1000;
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

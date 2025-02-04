
#include <gperftools/profiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <csignal>
#include <iostream>

void consumeSomeCPUTime1(int input) {
  int i = 0;
  input++;
  while (i++ < 100000) {
    i--;
    i++;
    i--;
    i++;
  }
};

void consumeSomeCPUTime2(int input) {
  input++;
  consumeSomeCPUTime1(input);
  int i = 0;
  while (i++ < 100000) {
    i--;
    i++;
    i--;
    i++;
  }
};

int stupidComputing(int a, int b) {
  int i = 0;
  while (i++ < 1000) {
    consumeSomeCPUTime1(i);
  }
  int j = 0;
  while (j++ < 5000) {
    consumeSomeCPUTime2(j);
  }
  return a + b;
};

int smartComputing(int a, int b) { return a + b; };

void signalHandler(int signum) {
  std::cout << "out:" << std::endl;
  // LOG(ERROR)<<" exit ====== ";
}

int main() {
  /*
    if (SIG_ERR == signal(SIGABRT, signalHandler)) {
      return 1;
    }
    if (SIG_ERR == signal(SIGILL, signalHandler)) {
      return 1;
    }
    if (SIG_ERR == signal(SIGTERM, signalHandler)) {
      return 1;
    }

    if (SIG_ERR == signal(SIGFPE, signalHandler)) {
      return 1;
    }
    */

  int i = 0;
  printf("reached the start point of performance bottle neck\n");
  sleep(5);
  // ProfilerStart("info.prof");
  while (i++ < 2) {
    printf("Stupid computing return : %d\n", stupidComputing(i, i + 1));
    printf("Smart computing return %d\n", smartComputing(i + 1, i + 2));
  }
  printf("should teminate profiling now.\n");
  sleep(5);
  // ProfilerStop();

  return 0;
}

#include "include/message.hh"

namespace message {

  void SetThreadName(const char* name) {
    int result = prctl(PR_SET_NAME, name, 0, 0, 0);
    if (result == 0) {
        std::cout << "Thread name set successfully: " << name << std::endl;
    } else {
        std::cerr << "Failed to set thread name: " << strerror(errno) << std::endl;
    }
  }

  int bindCpu(int i) {
    if (i < 0) {
      return -1;
    }
    cpu_set_t mask;
    CPU_ZERO(&mask);

    CPU_SET(i, &mask);

    if (-1 == pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask)) {
      fprintf(stderr, "pthread_setaffinity_np erro\n");
      return -1;
    }
    return 0;
  }


  std::atomic<bool> flag(false);
  SPSCQueue<Snap>  snap_spsc_queue(SNAP_QUEUE_SIZE);
  SPSCQueue<Index> index_spsc_queue(INDEX_QUEUE_SIZE);
}
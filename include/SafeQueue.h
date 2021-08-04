#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
using namespace std;

// A threadsafe-queue.
//template <class T>
class SafeQueue
{
public:
  SafeQueue(void);

  ~SafeQueue(void);

  // Add an element to the queue.
  void enqueue(string t);

  // Get the "front"-element.
  // If the queue is empty, wait till a element is avaiable.
  string dequeue(void);
  int getCount();

private:
  std::queue<string> q;
  mutable std::mutex m;
  std::condition_variable c;
};
#endif
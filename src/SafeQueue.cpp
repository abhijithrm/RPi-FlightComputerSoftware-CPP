#include <SafeQueue.h>

SafeQueue::SafeQueue(void)
    : q()
    , m()
    , c()
  {}

  SafeQueue::~SafeQueue(void)
  {}

  // Add an element to the queue.
  void SafeQueue::enqueue(string t)
  {
    std::lock_guard<std::mutex> lock(m);
    q.push(t);
    c.notify_one();
  }

  // Get the "front"-element.
  // If the queue is empty, wait till a element is avaiable.
  string SafeQueue::dequeue(void)
  {
    std::unique_lock<std::mutex> lock(m);
    while(q.empty())
    {
      // release lock as long as the wait and reaquire it afterwards.
      c.wait(lock);
    }
    string val = q.front();
    q.pop();
    return val;
  }

  int SafeQueue::getCount()
  {
    return q.size();
  }

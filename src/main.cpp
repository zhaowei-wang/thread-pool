#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>

#define WORK_POOL_COUNT 4

/*
OO threading model
 */
template <class host>
class object_lockable
{
public:
  // object is locked until lock class goes out-of-scope
  class lock
  {
  public:
    lock(host &h)
    {
      _alias_mutex = &h._mutex;
      _alias_mutex->lock();
    }
    ~lock() { _alias_mutex->unlock(); }
  private:
    std::mutex *_alias_mutex;
  };
  
protected:
  std::mutex _mutex;
};

template <class host>
class class_lockable
{
public:
  // class or object is locked until lock class goes out of scope
  class lock
  {
  public:
    lock()  { host::s_mutex.lock();   }
    ~lock() { host::s_mutex.unlock(); }
  };

protected:
  static std::mutex s_mutex;  
};

template <class host>
std::mutex class_lockable<host>::s_mutex;

template <class host>
class not_lockable
{
public:
  class lock
  {
  public:
    lock() {};
    lock(host &h) {};
    ~lock() {};
  };
};
/*
end
 */

/*
work: functor storing a function and its arguements for delayed
execution
 */

//helper to unpack std::tuple 
template <int ...>
struct seq {};

template <int N, int ...S>
struct generator : generator<N-1, N-1, S...> {};

template <int ...S>
struct generator<0, S...> { typedef seq<S...> type; }; // base case

template <class function_t, class ...args_t>
class work
{
public:
  work(function_t f, args_t... args)
    : _f(f), _args(std::forward<args_t>(args)...) {}

  auto operator()()
  {
    return execute_f(typename generator<sizeof...(args_t)>::type());
  }
  
private:
  function_t _f;
  std::tuple<args_t...> _args;

  template <int ...S>
  auto execute_f(seq<S...>)
  {
    return _f(std::get<S>(_args)...);
  }
};
/*
end
 */

/*
Concurrent Queue: queue with object level locking
 */
template <class T>
class cqueue : public object_lockable<cqueue<T>>
{
public:
  cqueue() : q(std::deque<T>()) {}
  
  void enq(T a)
  {
    olock l(*this);
    q.push_front(a);
  }

  void deq()
  {
    olock l(*this);
    q.pop_back();
  }

  T peek()
  {
    olock l(*this);
    return q.back();
  }

  size_t size() { return q.size(); }
private:
 std::deque<T> q;
 using olock = typename object_lockable<cqueue<T>>::lock;
};
/*
end
 */

/*
scheduler: wraps std::thread with a future/promise model
 */
class scheduler
{
public:
  scheduler() {}
  
private:
};
/*
end
*/

int main(int argc, char * argv[])
{
  std::thread threads[WORK_POOL_COUNT];

  cqueue<size_t> q;
  int N = 10000000;
  std::atomic<int> sum{0};
  
  auto f = [&] () mutable
	   {
	     for (size_t i = 0; i < N; ++i)
	       sum += 1;
	   };
    
  for (size_t i = 0; i < WORK_POOL_COUNT; ++i)
    threads[i] = std::thread(work<decltype(f)>(f));

  for (size_t i = 0; i < WORK_POOL_COUNT; ++i)
    threads[i].join();

  std::cout << sum << std::endl;
  
  return 0;
}

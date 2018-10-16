#include <iostream>
#include <memory>
#include <thread>
#include <mutex>

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
      alias_mutex_ = &h.mutex_;
      alias_mutex_->lock();
    }
    ~lock() { alias_mutex_->unlock(); }
  private:
    std::mutex *alias_mutex_;
  };
  
private:
  std::mutex mutex_;
};

template <class host>
class class_lockable
{
public:
  // class or object is locked until lock class goes out of scope
  class lock
  {
  public:
    lock() : alias_mutex_(nullptr)
    {
      smutex_.lock();
    }
    
    lock(host &h)
    {
      alias_mutex_ = &h.mutex_;
      alias_mutex_->lock();
    }
    
    ~lock()
    {
      alias_mutex_->unlock();
      smutex_.unlock();
    }

  private:
    std::mutex *alias_mutex_;
  };
  
private:
  std::mutex mutex_;
  static std::mutex smutex_;  
};

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
Work scheduler
 */
template <class work_t, class arg_t, class callback_t>
class work
{
public:
  work() : unit(nullptr), arg(nullptr), callback(nullptr) {}
  work(work_t u, arg_t a, callback_t c) : unit(u), arg(a), callback(c) {}
  
  void operator()()
  {
    callback(work(arg));
  }
private:
  work_t unit;
  arg_t arg;
  callback_t callback;
};

template<class work_t>
class work<work_t, void, void>
{
public:
  work() : unit(nullptr) {}
  work(work_t u) : unit(u) {}
  
  void operator()()
  {
    unit();
  }
private:
  work_t unit;
};
/*
end
 */

class concurrent_int : public object_lockable<concurrent_int>
{
public:
  concurrent_int() : i(0) {};
  
  void increment()
  {
    lock l(*this);
    i++;
  }

  int value() { return i; }
  
private:
  int i;
};

int main(int argc, char * argv[])
{
  std::thread threads[WORK_POOL_COUNT];
  work<std::function<void()>, void, void> to_do[WORK_POOL_COUNT];

  concurrent_int ci;
  int N = 1000000;
  
  std::function<void()> f = [&] ()
	   {
	     for (size_t i = 0; i < N; ++i)
	       ci.increment();
	   };
  
  for (size_t i = 0; i < WORK_POOL_COUNT; ++i)
  {
    to_do[i] = work<std::function<void()>, void, void>(f);
  }
  
  for (size_t i = 0; i < WORK_POOL_COUNT; ++i)
    threads[i] = std::thread(to_do[i]);

  for (size_t i = 0; i < WORK_POOL_COUNT; ++i)
    threads[i].join();

  std::cout << "expected " << WORK_POOL_COUNT * N << " ";
  std::cout << "got " << ci.value() << std::endl;
  
  return 0;
}

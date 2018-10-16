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
      host::smutex_.lock();
    }
    
    lock(host &h)
    {
      alias_mutex_ = &h.mutex_;
      alias_mutex_->lock();
    }
    
    ~lock()
    {
      alias_mutex_->unlock();
      host::smutex_.unlock();
    }

  private:
    std::mutex *alias_mutex_;
  };

private:
  std::mutex mutex_;
  static std::mutex smutex_;  
};

template <class host>
std::mutex class_lockable<host>::smutex_;

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
Work: class and its specializations represent units of work, to be
executed concurrently
 */
template <class work_t, class arg_t, class callback_t>
class work
{
public:
  work() {}
  work(work_t u, arg_t a, callback_t c) : unit(u), arg(a), callback(c) {}
  void operator()() { callback(work(arg)); }
  
private:
  work_t unit;
  arg_t arg;
  callback_t callback;
};

template<class work_t>
class work<work_t, void, void>
{
public:
  work() {}
  work(work_t u) : unit_(u) {}
  void operator()() { unit_(); }
  
private:
  work_t unit_;
};

template<class work_t, class arg_t>
class work<work_t, arg_t, void>
{
public:
  work() {}
  work(work_t u, arg_t a) : unit_(u), arg_(a) {}

  void operator()() { unit_(arg_); }
  
private:
  work_t unit_;
  arg_t arg_;
};

template<class work_t, class callback_t>
class work <work_t, void, callback_t>
{
public:
  work() {}
  work(work_t u, callback_t c) : unit_(u), callback_(c) {}
  void operator()() { callback_(unit_()); }
  
private:
  work_t unit_;
  callback_t callback_;
};
/*
end
 */

class concurrent_int : public class_lockable<concurrent_int>
{
public:
  concurrent_int() : i(0) {};
  
  void increment()
  {
    lock l;
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

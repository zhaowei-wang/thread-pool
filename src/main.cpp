#include <iostream>
#include <pthread.h>

#define WORK_POOL_COUNT 4

/*
OO threading model
 */
template <class host>
class object_lockable
{
public:
  class lock
  {
  public:
    lock(host &h)  { pthread_mutex_lock(&mutex_); }
    ~lock()        { pthread_mutex_unlock(&mutex_); }

  private:
    pthread_mutex_t mutex_;
  };
};

template <class host>
class class_lockable
{
public:
  class lock
  {
  public:
    lock()        { pthread_mutex_lock(&smutex_); }
    lock(host &h) { pthread_mutex_lock(&mutex_); }
    ~lock()       { pthread_mutex_unlock(&mutex_);
                    pthread_mutex_unlock(&smutex_); }

  private:
    pthread_mutex_t mutex_;
    static pthread_mutex_t smutex_;
  };
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
  work(work_t *u, arg_t *a, callback_t *c) : unit(u), arg(a), callback(c) {}
  
  void operator()()
  {
    if (callback && arg)
    {
      (*callback)((*unit)(*arg));
      return;
    }
    else if (callback && !arg)
    {
      (*callback)((*unit)());
      return;
    }
    else if (!callback && arg)
    {
      (*unit)(*arg);
      return;
    }
    
    (*unit)();
  }
private:
  work_t *unit;
  arg_t *arg;
  callback_t *callback;
};
/*
end
 */

class concurrent_int : object_lockable<concurrent_int>
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

void increment_ci(concurrent_int *ci)
{
  ci->increment();
}

int main(int argc, char * argv[])
{
  pthread_t threads[WORK_POOL_COUNT];
  work<std::function<void()>, void, void> to_do[WORK_POOL_COUNT];

  concurrent_int ci;
  int N = 100;
  
  std::function<void()> f = [&] ()
	   {
	     for (size_t i = 0; i < N; ++i)
	       ci.increment();
	   };
  
  for (size_t i = 0; i < WORK_POOL_COUNT; ++i)
  {
    to_do[i] = work<std::function<void()>, void, void>(&f, nullptr, nullptr);
  }
  
  for (size_t i = 0; i < WORK_POOL_COUNT; ++i)
    pthread_create(&threads[i], nullptr, to_do[i], nullptr);

  for (size_t i = 0; i < WORK_POOL_COUNT; ++i)
    pthread_join(&threads[i], nullptr);

  std::cout << "expected " << WORK_POOL_COUNT * N << " ";
  std::cout << "got " << ci.value() << std::endl;
  
  return 0;
}

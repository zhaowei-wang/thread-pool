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

class concurrent_int : object_lockable<concurrent_int>
{
public:
  concurrent_int() : i(0) {};
  
  void increment()
  {
    lock l(*this);
    i++;
  }
  
private:
  int i;
};

int main(int argc, char * argv[])
{
  std::cout << "Hello world!" << std::endl;
  return 0;
}

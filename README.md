# cpp11_thread_utils
Utility classes for std::thread library.

## Requirements
* at least C++11
* _pthread_ or (MinGW-w64) _winpthreads_

## Classes
* _Thread_ - A wrapper class around std::thread with extended functionality like:
  * cancel
  * kill
  * detach
  * set priority
* _Semaphore_ - Header only semaphore implementation using std::condition_variable
* _PosixSemaphore_ - Semaphore implementation using POSIX sem_post, sem_wait
* _ConditionMutex_ - A mutex and condition_variable in one piece. Implements _'Lockable'_ concept.

## Example
```c++

thread_utils::Thread thread("thread_0");

auto thread_function = [&quit_flag]()
{
    uint32_t counter = 1;
    while(quit_flag.load())
    {
        thread_utils::sleepFor(1000);
        printf("thread_0 is running for %u\n", counter++);
    }
}

thread.run(thread_function);
```

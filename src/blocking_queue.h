#ifndef _BLOCKING_QUEUE_H_
#define _BLOCKING_QUEUE_H_

#include "semaphore.h"
#include <queue>
#include <mutex>
#include <optional>

/**
 * Example: 
 * 
 *      thread_utils::BlockingQueue<uint64_t> data_queue;
 * 
 *      std::atomic_bool is_running(true);
 *      std::thread th([&is_running, &data_queue]()
 *      {
 *          while(is_running.load())
 *          {
 *              auto data = data_queue.pop();
 *              printf("New data: %lu\n", data.value());
 *          }
 *      });
 * 
 *      size_t number_of_data = std::experimental::randint(100, 999);
 *      for(size_t i = 0; i < number_of_data; ++i)
 *      {
 *          //generates a random uint64_t number from 0 to uint64_t MAX
 *          auto random_data = std::experimental::randint<uint64_t>(0, std::numeric_limits<uint64_t>::max());
 *          //waits for a random number of milliseconds
 *          std::this_thread::sleep_for(std::chrono::duration<int64_t, std::milli>(std::experimental::randint<uint64_t>(100, 3000)));
 *          //add random data to queue
 *          data_queue.push(random_data);//or data_queue.emplace(random_data);
 *      }
 *      is_running.store(false);
 *      if(th.joinable())
 *      { th.join(); }
 * 
 * Example 1:
 * 
 *      Same as above
 *      ...
 *          while(is_running.load())
 *          {
 *              if( auto data = data_queue.pop(1500) )//timeout is 1500 milliseconds
 *              {
 *                  printf("New data: %lu\n", data.value());
 *              } else {
 *                  printf("There was no data for at least 1500 milliseconds\n");
 *              }
 *          }
 *      ...
 * 
 */

namespace thread_utils
{
    template<typename T>
    class BlockingQueue
    {
    private:
        std::mutex      mMutex;
        std::deque<T>   mQueue;
        semaphore_t     mQueueSemaphore;
    public:
        BlockingQueue() {}
        /**
         * Push an element into the queue
         * Copies the given value!
         * @param element A const reference value of type T
         * @param front If true is given then the element is pushed to front instead of back
         */
        void push(const T& element, bool front = false)
        {
            std::lock_guard<std::mutex> guard(mMutex);
            if(front)   { mQueue.push_front(element); } 
            else        { mQueue.push_back(element); }
            mQueueSemaphore.post();
        }
         /**
         * Emplace an element into the queue
         * Moves the given value!
         * @param element An rvalue of type T
         */
        void emplace(T&& element)
        {
            std::lock_guard<std::mutex> guard(mMutex);
            mQueue.emplace(element);
            mQueueSemaphore.post();
        }
        /**
         * Pops and returns the last element. This function is blocking while there is no element in the queue.
         * @param timeout_ms The maximum amount of milliseconds to wait while the queue is empty. If the value is equal or
         * lesser than 0 it will wait forever. Default value: -1
         * @return If the given time has passed an std::nullopt is returned, otherwise a value of type T is returned.
         */
        std::optional<T> pop(int64_t timeout_ms = -1)
        {
            if( timeout_ms > 0 )
            {
                if( !mQueueSemaphore.wait_for(timeout_ms) ) 
                { return std::nullopt; }
            } else {
                mQueueSemaphore.wait();
            }
            std::lock_guard<std::mutex> guard(mMutex);
            //There is no need to check if the queue is empty thankfully to the semaphore.
            auto element = std::make_optional<T>(std::move(mQueue.front()));
            mQueue.pop_front();
            return element;
        }
        /**
         * Clears the queue
         */
        void clear()
        {
            std::lock_guard<std::mutex> guard(mMutex);
            mQueue.clear();
        }
    };

    template<typename T>
    class BlockingSlot
    {
    private:
        std::mutex          mMutex;
        std::optional<T>    mSlot;
        binary_semaphore_t  mSemaphore;
    public:
        BlockingSlot() : mMutex(), mSlot(std::nullopt), mSemaphore() {}
        /**
         * Sets the given value to the slot
         * @param value
         * @return True is returned if the slot was empty, otherwise false
         */
        bool set(const T& value)
        {
            std::lock_guard<std::mutex> guard(mMutex);
            mSlot = value;
            return mSemaphore.post();
        }
        /**
         * Returns the value of the slot if set prevoiusly
         * This function is blocking until the slot set or the given timeout expires
         * @param timeout_ms Timeout in millseconds. For an unlimited timeout, use value -1
         * @return std::nullopt is returned on failure
         */
        std::optional<T> get(int64_t timeout_ms = -1)
        {
            if(timeout_ms < 0)
            {
                mSemaphore.wait();
            } else {
                if( !mSemaphore.wait_for(timeout_ms) )
                { return std::nullopt; }
            }
            std::lock_guard<std::mutex> guard(mMutex);
            return mSlot;
        }
        /**
         * Clears the slot
         */
        void clear()
        {
            std::lock_guard<std::mutex> guard(mMutex);
            mSlot = std::nullopt;
        }
    };
}

#endif

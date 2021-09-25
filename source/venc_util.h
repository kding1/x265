// Copyright 2018, Intel Corporation. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the copyright holder nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef util_h
#define util_h

#include <queue>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

// naive synchronized queue implementation
template<typename T>
class sync_queue {
public:
    sync_queue()
    {
        timeout_ms = 0;
    }

    T get()
    {
        std::unique_lock<std::mutex> lock(mtx);
        while (q.empty()) {
            cond.wait(lock);
        }
        
        auto element = q.front();
        q.pop_front();
        return element;
    }

    bool get(T& element)
    {
        std::unique_lock<std::mutex> lock(mtx);
        std::cv_status ret = std::cv_status::no_timeout;
        while (q.empty()) {
            if (timeout_ms == 0) {
                cond.wait(lock);
            } else {
                ret = cond.wait_for(lock,
                      std::chrono::milliseconds(timeout_ms));
                if (ret == std::cv_status::timeout) {
                    break;
                }
            }
        }

        if (ret == std::cv_status::no_timeout) {
            element = q.front();
            q.pop_front();
            return true;
        } else {
            return false;
        }
    }
    
    void put(T& element)
    {
        std::unique_lock<std::mutex> lock(mtx);
        q.push_back(element);
        lock.unlock();
        cond.notify_one();
    }

    void put_front(T& element)
    {
        std::unique_lock<std::mutex> lock(mtx);
        q.push_front(element);
        lock.unlock();
        cond.notify_one();
    }
    
    int size()
    {
        return (int)(q.size());
    }
    
    void set_timeout(int ms)
    {
        timeout_ms = ms;
    }

private:
    int timeout_ms;
    std::deque<T> q;
    std::mutex mtx;
    std::condition_variable cond;
};


class semaphore {
public:
    semaphore(int ms = 0)
    {
        // intial state is unlocked.
        count = 1;
        timeout_ms = (ms >= 0) ? ms : 0;
    }

    bool acquire()
    {
        std::unique_lock<std::mutex> lock(mtx);
        std::cv_status ret = std::cv_status::no_timeout;

        while (count <= 0) {
            if (timeout_ms == 0) {
                cond.wait(lock);
            } else {
                ret = cond.wait_for(lock,
                      std::chrono::milliseconds(timeout_ms));
                if (ret == std::cv_status::timeout) {
                    break;
                }
            }
        }

        if (ret == std::cv_status::no_timeout) {
            count--;
            return true;
        } else {
            return false;
        }
    }

    void release()
    {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        lock.unlock();
        cond.notify_one();
    }

    void set_timeout(int ms)
    {
        timeout_ms = ms;
    }

private:
    int timeout_ms;
    int count;
    std::mutex mtx;
    std::condition_variable cond;
};

#endif /* util_h */


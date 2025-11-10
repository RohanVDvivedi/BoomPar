# BoomPar
This is a multi threading library in C, built as an abstraction layer over pthread (POSIX threads) library.
It provides 3 user exported constructs an Executor, that can be either FIXED_THREAD_COUNT_EXECUTOR or a CACHED_THREAD_POOL_EXECUTOR, managing variable number of threads.
Next it provides is a periodic_job, a construct that allows you to control and manage a job that is meant to run at triggers or in a periodic fashion at fixed (but updatable intervals).
Finally there is an alarm_job, very similar to the periodic_job except that it gets it's next waiting time from the job itself (specifically designed for removing/evicting old elements from the data structures based on their expiry times, like in some cache systems).

### Features
 * job: A wrapper to submit a task to be done, with a promise for completion.
 * executor: A (cached or fixed-thread-count) thread pool based job execution API, similar to Java's [ExecutorService](https://docs.oracle.com/javase/7/docs/api/java/util/concurrent/ExecutorService.html).

## Setup instructions
**Install dependencies :**
 * [Cutlery](https://github.com/RohanVDvivedi/Cutlery)
 * [PosixUtils](https://github.com/RohanVDvivedi/PosixUtils)

**Download source code :**
 * `git clone https://github.com/RohanVDvivedi/BoomPar.git`

**Build from source :**
 * `cd BoomPar`
 * `make clean all`

**Install from the build :**
 * `sudo make install`
 * ***Once you have installed from source, you may discard the build by*** `make clean`

## Using The library
 * add `-lboompar -lpthread -lcutlery` linker flag, while compiling your application
 * do not forget to include appropriate public api headers as and when needed. this includes
   * `#include<boompar/job.h>`
   * `#include<boompar/promise.h>`
   * `#include<boompar/executor.h>`
   * `#include<boompar/periodic_job.h>`
   * `#include<boompar/alarm_job.h>`
   * `#include<boompar/resource_usage_limiter.h>`
   * `#include<boompar/tiber.h>` a very thin fiber library over ucontext, resembling fibers/coroutines as in other programming languages 

   * `#include<boompar/sync_pipe.h>` can be used to share data among different jobs running on separate threads.

   * `#include<boompar/worker.h>` and `#include<boompar/job_queue.h>` api header are ony required if you want to build your own custom executor, *which may not be necessary*.

## Instructions for uninstalling library

**Uninstall :**
 * `cd BoomPar`
 * `sudo make uninstall`

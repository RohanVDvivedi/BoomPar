# BoomPar
This is a multi threading library in C, built as an abstraction layer over pthread (POSIX threads) library.

### Features
 * job: A wrapper to submit a task to be done, with a promise for completion.
 * executor: A (cached or fixed-thread-count) thread pool based job execution API, similar to Java's [ExecutorService](https://docs.oracle.com/javase/7/docs/api/java/util/concurrent/ExecutorService.html).

## Setup instructions
**Install dependencies :**
 * [Cutlery](https://github.com/RohanVDvivedi/Cutlery)

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
   * `#include<job.h>`
   * `#include<executor.h>`

   * `#include<worker.h>` and `#include<sync_queue.h>` api header are ony required if you want to built your own custom executor.

## Instructions for uninstalling library

**Uninstall :**
 * `cd BoomPar`
 * `sudo make uninstall`

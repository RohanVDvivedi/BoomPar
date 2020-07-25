# BoomPar
This is a multi threading library in C, built as an abstraction layer over pthread (POSIX threads) library.

Features
 * job: A wrapper to submit a task to be done, with a promise for completion.
 * executor: A (cached or fixed-thread-count based) thread pool based job execution API, similar to Java's [ExecutorService](https://docs.oracle.com/javase/7/docs/api/java/util/concurrent/ExecutorService.html).

setup instructions
 * git clone https://github.com/RohanVDvivedi/BoomPar.git
 * cd BoomPar
 * sudo make clean install
 * add "-lboompar -lpthread -lcutlery" linker flag, while compiling your application
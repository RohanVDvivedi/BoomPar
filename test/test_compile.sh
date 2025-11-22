gcc -Wall test_periodic_job.c     -o test_periodic_job.out     -lboompar -lpthread -lcutlery
gcc -Wall test_alarm_job.c        -o test_alarm_job.out	       -lboompar -lpthread -lcutlery
gcc -Wall test_executor.c         -o test_executor.out         -lboompar -lpthread -lcutlery
gcc -Wall test_job.c              -o test_job.out              -lboompar -lpthread -lcutlery
gcc -Wall test_worker.c           -o test_worker.out           -lboompar -lpthread -lcutlery
gcc -Wall test_resource_limiter.c -o test_resource_limiter.out -lboompar -lpthread -lcutlery

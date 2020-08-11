gcc -Wall test_executor.c -o test_executor.out	-lboompar -lpthread -lcutlery
gcc -Wall test_job.c 	  -o test_job.out		-lboompar -lpthread -lcutlery
gcc -Wall test_worker.c   -o test_worker.out	-lboompar -lpthread -lcutlery

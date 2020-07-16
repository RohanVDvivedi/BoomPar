cd ..
make all
cd test
gcc -Wall test_executor.c -o test_executor.out	-lpthread -lboompar -lcutlery
gcc -Wall test_job.c 	  -o test_job.out		-lpthread -lboompar
gcc -Wall test_worker.c   -o test_worker.out	-lpthread -lboompar -lcutlery

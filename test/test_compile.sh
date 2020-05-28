cd ..
make all
cd test
gcc test_executor.c -o test_executor.out	-I../inc -I../../cutlery/inc -L../bin -L../../cutlery/bin -lpthread -lboompar -lcutlery
gcc test_job.c 		-o test_job.out			-I../inc -I../../cutlery/inc -L../bin -L../../cutlery/bin -lpthread -lboompar -lcutlery
gcc test_worker.c 	-o test_worker.out		-I../inc -I../../cutlery/inc -L../bin -L../../cutlery/bin -lpthread -lboompar -lcutlery

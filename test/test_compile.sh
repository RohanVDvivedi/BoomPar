cd ..
make all
cd test
gcc test.c -I../inc -I../../cutlery/inc -L../bin -L../../cutlery/bin -lpthread -lboompar -lcutlery

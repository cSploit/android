# Test for library integrity
# this should be compiled with optimization turned off
ulimit -s unlimited
ulimit -c unlimited
g++ -ggdb -Wall -I../../include -DTESTING_ONLY -DDEV_BUILD class_test.cpp alloc.cpp ../fb_exception.cpp 2> aa
#valgrind --tool=memcheck ./a.out

#export LD_LIBRARY_PATH=/usr/local/lib64

# Chose the best algorithm parameters for the target architecture
#/usr/local/bin/
#g++ -ggdb -O3 -DHAVE_STDLIB_H -DTESTING_ONLY \
#-I../../include class_perf.cpp alloc.cpp ../fb_exception.cpp 2> aa
./a.out

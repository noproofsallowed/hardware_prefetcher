CC = g++
CCFLAGS = -g 

all: cacheSim sampleSim emptySim

cacheSim: CPU.o cache.o cache_main.o memQueue.o prefetcher.o 
	${CC} ${CCFLAGS} CPU.o cache.o cache_main.o memQueue.o prefetcher.o -o bin/cacheSim 

sampleSim: CPU.o cache.o sample_main.o memQueue.o sample_prefetcher.o
	${CC} ${CCFLAGS} CPU.o cache.o sample_main.o memQueue.o sample_prefetcher.o -o bin/sampleSim

emptySim:  CPU.o cache.o empty_main.o memQueue.o empty_prefetcher.o 
	${CC} ${CCFLAGS} CPU.o cache.o empty_main.o memQueue.o empty_prefetcher.o -o bin/emptySim

CPU.o: CPU.C CPU.h mem-sim.h
	${CC} ${CCFLAGS} -c CPU.C

cache.o: cache.C cache.h
	${CC} ${CCFLAGS} -c cache.C

cache_main.o: main.C mem-sim.h CPU.h cache.h memQueue.h prefetcher.h empty_prefetcher.h sample-pf/prefetcher.h
	${CC} ${CCFLAGS} -c main.C -D cache -o cache_main.o

sample_main.o: main.C mem-sim.h CPU.h cache.h memQueue.h prefetcher.h empty_prefetcher.h sample-pf/prefetcher.h
	${CC} ${CCFLAGS} -c main.C -D sample -o sample_main.o -I .

empty_main.o: main.C mem-sim.h CPU.h cache.h memQueue.h prefetcher.h empty_prefetcher.h sample-pf/prefetcher.h
	${CC} ${CCFLAGS} -c main.C -D empty -o empty_main.o

memQueue.o: memQueue.C memQueue.h mem-sim.h cache.h
	${CC} ${CCFLAGS} -c memQueue.C

prefetcher.o: prefetcher.C prefetcher.h mem-sim.h
	${CC} ${CCFLAGS} -c prefetcher.C

empty_prefetcher.o: empty_prefetcher.C empty_prefetcher.h mem-sim.h
	${CC} ${CCFLAGS} -c empty_prefetcher.C

sample_prefetcher.o: sample-pf/prefetcher.C sample-pf/prefetcher.h
	${CC} ${CCFLAGS} -c sample-pf/prefetcher.C -I . -o sample_prefetcher.o

clean:
	rm -f *.o bin/*

# MGSim
CC = slc -b mipsel_s -DCUSTOM_RANDOM
RUN_COMMAND = slr qsort 2000

# Simple Scalar
#CC = xgcc
#RUN_COMMAND = sim-outorder qsort 2000

all: qsort

run: qsort
	$(RUN_COMMAND)

clean:
	rm -f qsort


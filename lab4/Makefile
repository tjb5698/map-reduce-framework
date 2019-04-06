OUT = mr-wordc
OUTS = mr-wordc mr-trace mr-sleep mapreduce.o
CFLAGS = -Wall -g -std=gnu99 -pthread
SOURCES = mapreduce.o
LIBS =

tests = 1.txt 2.txt

INPUT_PATH=input
OUTPUT_PATH=output
TRUTH_PATH=output_compare

t=5

.PHONY: default

default: $(OUTS)

mr-wordc: $(SOURCES) bin/mr-wordc.o
	gcc $(CFLAGS) $^ $(LIBS) -o $@
	mkdir -p output

mr-trace: $(SOURCES) bin/mr-trace.o 
	gcc $(CFLAGS) $^ $(LIBS) -o $@

mr-sleep: $(SOURCES) bin/mr-sleep.o 
	gcc $(CFLAGS) $^ $(LIBS) -o $@

mapreduce.o: mapreduce.c
	gcc $(CFLAGS) -c $^ $(LIBS) -o $@ 

clean:
	rm -f $(OUTS)

tests:
	for test in $(TESTS); do
		./$(OUT) $(INPUT_PATH)/$$test $(OUTPUT_PATH)/$$test $t || exit 1
	done

compare:
	for test in $(TESTS); do
		python3 ../compare.py $(OUTPUT_PATH)/$$test $(TRUTH_PATH)/$$test
	done

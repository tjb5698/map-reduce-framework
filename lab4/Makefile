OUT = mr-wordc
OUTS = mr-wordc mr-trace mr-sleep
CFLAGS = -std=gnu99 -pthread
SOURCES = mapreduce.c
LIBS =

tests = 1.txt 2.txt

INPUT_PATH=input
OUTPUT_PATH=output
TRUTH_PATH=output_compare

t=5

.PHONY: default

default: $(OUTS)

mr-wordc: $(SOURCES)
	gcc -Wall -g $(CFLAGS) $^ bin/mr-wordc.o $(LIBS) -o $@
	mkdir -p output

mr-trace: $(SOURCES)
	gcc -Wall -g $(CFLAGS) $^ bin/mr-trace.o $(LIBS) -o $@

mr-sleep: $(SOURCES)
	gcc -Wall -g $(CFLAGS) $^ bin/mr-sleep.o $(LIBS) -o $@

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

OUT = wordcount
CFLAGS = -std=gnu99
SOURCES = mapreduce.c wordcount.o
LIBS = 

tests = 1.txt 2.txt

INPUT_PATH=input
OUTPUT_PATH=output
TRUTH_PATH=output_compare

t=5

.PHONY: default

default: mapreduce.h
	gcc -Wall -g $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
	mkdir -p output

clean:
	rm $(OUT)

tests: 
	for test in $(TESTS); do 
		./$(OUT) $(INPUT_PATH)/$$test $(OUTPUT_PATH)/$$test $t || exit 1
	done
	
compare:
	for test in $(TESTS); do 
		python3 ../compare.py $(OUTPUT_PATH)/$$test $(TRUTH_PATH)/$$test
	done

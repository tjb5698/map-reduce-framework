OUT = wordcount
CFLAGS = -std=gnu99
SOURCES = mapreduce.c wordcount.o
LIBS = 

t=1

INPUT_PATH=input

.PHONY: default

default: mapreduce.h
	gcc -Wall -g $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)

clean:
	rm $(OUT)

run:
	./$(OUT) $(INPUT_PATH)/$t.txt


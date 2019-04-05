# Compare the two output files for checking correctness. 

import csv
import sys
import os

if len(sys.argv) < 3:
    print("Usage: %s [input1] [input2] \n\t\tto check whether the two inputs are equivalent\n" % sys.argv[0])
    

print("\nTesting %s:\n" % sys.argv[1].split("/")[-1])

correct = True
with open(sys.argv[1]) as f1:
    with open(sys.argv[2]) as f2:
        d1 = {r[0]: int(r[1]) for r in csv.reader(f1)}
        d2 = {r[0]: int(r[1]) for r in csv.reader(f2)}
        for k in d1.keys():
            if k not in d2:
                print("%s is not found in %s\n" % (k, sys.argv[2]))
                correct = False
            elif d1[k] != d2[k]:
                print("%s counts differently as (%d, %d)\n" % (k, d1[k], d2[k]))
                correct = False
            else: 
                del d1[k]
                del d2[k]
        for k in d2:
            if k not in d1:
                print("%s is not found in %s\n" % (k, sys.argv[1]))
                correct = False
        if correct:
            print("Correct, test %s passed.\n" % sys.argv[1].split("/")[-1])
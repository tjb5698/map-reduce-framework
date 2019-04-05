# Compare the two output files for checking correctness. 

import csv
import sys
import os

if len(sys.argv) < 3:
    print("Usage: %s [input1] [input2] \n\t\tto check whether the two inputs are equivalent" % sys.argv[0])
        
os.system("(echo \"word, count\" && cat %s) > /tmp/1" %sys.argv[1])
os.system("(echo \"word, count\" && cat %s) > /tmp/2" %sys.argv[2])
    
correct = True
with open("/tmp/1", newline='') as f1:
    with open("/tmp/2", newline='') as f2:
        d1 = {r[0]: int(r[1]) for r in csv.reader(f1)}
        d2 = {r[0]: int(r[1]) for r in csv.reader(f2)}
        for k in d1:
            if k not in d2:
                print("%s is not found in %s\n" % (k, sys.argv[2]))
            elif d1[k] != d2[k]:
                print("%s counts differently as (%d, %d)\n" % (k, d1[k], d2[k]))
            else: 
                del d1[k]
                del d2[k]
        for k in d2:
            print("%s is not found in %s\n" % (k, sys.argv[1]))
        if correct:
            print("The two files are equivalent.\n")
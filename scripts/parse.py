import glob
import os

os.chdir(r'../build/bin/')


import re
numbers = re.compile(r'(\d+)')
def numericalSort(value):
    parts = numbers.split(value)
    parts[1::2] = map(int, parts[1::2])
    return parts

if os.path.exists("averages.data"):
    os.remove("averages.data")
else:
    print("averages.data file does not exist")

myFiles = sorted(glob.glob('*.txt'), key=numericalSort)

with open("averages.data", "a") as totFile:
    for name in myFiles:
        f = open(name, "r")
        lastLine = ""
        for x in f:
            lastLine = x;
        if lastLine != "":
            avg = lastLine.split(",")[0];
            totFile.write(name.split('.')[0] + "," + avg + '\n')


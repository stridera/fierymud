import os, sys 
from pathlib import Path

file = open('index', 'r')
icars = []
char_file = []
count = 0
  
## argument handling 
# print(len(sys.argv[1]))
if (len(sys.argv)) != 2 : quit("\nONE argument needed dipstick\nArgument needed for directory to scan, single capital character, A-Z please \n")

if (len(sys.argv[1])) != 1 : quit("\nSingle character num-nuts\nArgument needed for directory to scan, single capital character, A-Z please \n")

string = sys.argv[1]
if string.islower() :
    print ("\nDoes",sys.argv[1], "LOOK like a upper case character genius")
    quit ("Argument needed for directory to scan, single capital character, A-Z please \n")

if string.isnumeric() :
    print ("\nDoes",sys.argv[1], "LOOK like a upper case letter genius")
    quit ("Argument needed for directory to scan, single capital character, A-Z please \n")




#### processing stuff now.
for line in file:
    if len(line) > 5 :
        cars=line.split()
        #print(cars[1])
        icars.append(cars[1])
  
# Closing files
file.close() 

#get character files into an array

for files in [plr for plr in os.listdir(sys.argv[1]) 
if plr.endswith(".plr")]:
     fn = files.split(".")
     char_file.append(fn[0])
     #print(fn[0])
     if fn[0] not in icars : print(fn[0])  

# print(char_file)
# print(icars) 

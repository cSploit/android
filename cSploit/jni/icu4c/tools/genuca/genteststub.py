#!/usr/bin/python
#   Copyright (C) 2009, International Business Machines
#   Corporation and others.  All Rights Reserved.
#
#   file name:  genteststub.py
#   encoding:   US-ASCII
#   tab size:   8 (not used)
#   indentation:2

__author__ = "Claire Ho"

import sys

# read file
print "command:  python genteststub.py <inputFileName.txt>  <outputFileName.txt>"
print "if the fileName.txt is omitted, the default data file is CollationTest_NON_IGNORABLE_SHORT.txt"
if len(sys.argv) >= 2:
  fname=sys.argv[1]
else :
  fname="CollationTest_NON_IGNORABLE_SHORT.txt"
openfile = open(fname, 'r');

#output file name
ext=fname.find(".txt");
if len(sys.argv) >=3 :
  wFname = sys.argv[2]
elif (ext>0) :
  wFname = fname[:ext+1]+"_STUB.txt"
else :
  wFname = "out.txt"
wrfile = open(wFname,'w')

print "Reading file: "+fname+" ..."
print "Writing file: "+wFname+" ..."
count=10
for line in openfile.readlines():
  pos = line.find("#")
  if pos == 0:
    # print the header
    wrfile.write(line.rstrip()+"\n")
    continue
  if pos >= 0: line = line[:pos]
  line = line.rstrip()
  if line:
    if (count==10):
      wrfile.write(line+"\n")
      count=0
    count=count+1
if count!=1:
  if line:
    wrfile.write(line+"\n")  # write the last case
wrfile.close()
openfile.close()

#!/usr/bin/awk
#
# Convert input text into a C string
#
{
  gsub(/\"/,"\\\"");
  print "\"" $0 "\\n\"";
}

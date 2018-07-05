#!/bin/sh
# https://unix.stackexchange.com/questions/48750/creating-numerous-directories-using-mkdir

n=0;
max=42;
while [ "$n" -le "$max" ]; do
  mkdir "$n"
  n=`expr "$n" + 1`;
done

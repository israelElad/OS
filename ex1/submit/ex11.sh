#Elad Israel 313448888
#!/bin/bash
#search for whole word in file, and output with line number(truncate ":" with space) 
file=$1
word=$2
grep $word -w -n $file | tr ":" " "
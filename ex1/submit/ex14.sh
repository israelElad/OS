#Elad Israel 313448888
#!/bin/bash
#search for received client name in Bank-Log file received, print all lines in which the client appears, and sums up its balance.
name=$1
file=$2
nameLength=${#name}
#print all lines containing its name
grep "$name" -w $file
#iterate through all lines containing its name, start sum at 0, add the third word(client cash operation) to the sum, and at the end - print the sum.
grep "$name" -w $file | awk 'BEGIN {sumNumbers=0}{sumNumbers+=$3}END {print "Total balance: " sumNumbers}'
#Elad Israel 313448888
#!/bin/bash
#prints all files and directories in the path given with an appropriate suffix(file/folder) , lexicographically.
workingDirectory=$(pwd)
directory=$1
cd $directory
#find txt files, sort them, and add the suffix
ls | find . -maxdepth 1 -mindepth 1 -type f -name "*.txt" | cut -c3- | LC_ALL=C sort | sed 's/$/ is a file/'
#find directories, sort them, and add the suffix
ls | find . -maxdepth 1 -mindepth 1 -type d | cut -c3- | LC_ALL=C sort | sed 's/$/ is a directory/'
#change back to the previous working directory
cd $workingDirectory
#Elad Israel 313448888
#!/bin/bash
#search(lexicographically) for files in 2 depth levels with identical name to the one received. if found- print its content.
workingDirectory=$(pwd)
directory=$1
fileName=$2
cd "$directory"
#iterate over each item in the upper level
for upperItem in $(ls | LC_ALL=C sort)
do
	#if file
    if [[ -f "$upperItem" ]]
    then
            if [[ "$upperItem" == "$fileName" ]]
            then
            	#print its content
                cat "$upperItem"        
            fi
    #if folder        
    else
    		cd "$upperItem"
    		#iterate over each item in the lower level
            for lowerItem in $(ls | LC_ALL=C sort)
            do
                    if [[ ($lowerItem == $fileName) && ( -f $lowerItem) ]]
                    then
                    	#print its content
                        cat "$lowerItem"
            		fi
            done
            #get back to the upper level
            cd ..		
    fi                                  
done
#change back to the previous working directory
cd "$workingDirectory"
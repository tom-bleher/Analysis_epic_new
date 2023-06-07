#!/bin/bash
# 01/06/23 - Stephen Kay, University of York
# Script to combine output after it runs into a single EICRecon file
# Run this from the directory containing all of the output folders (i.e. within /volatile or /cache or whereever the jobs were outputting to

NumFiles=$1
if [[ -z "$1" ]]; then
    echo "I need to know the number of files that were processed so I can combine them!"
    echo "Please provide a number of files as the first argument."
    exit 1
fi

NumEvents=$2
if [[ -z "$2" ]]; then
    echo "I need to know the number of events that were processed for each file so I can combine them!"
    echo "Please provide a number of events per file as the second argument."
    exit 2
fi

# Check if an argument was provided for Egamma_start, if not, set 10
if [[ -z "$3" ]]; then
    Egamma_start=10
    echo "Egamma_start not specified, defaulting to 10"
else
    Egamma_start=$3
fi

# Check if an argument was provided for Egamma_end, if not, set 10
if [[ -z "$4" ]]; then
    Egamma_end=10
    echo "Egamma_end not specified, defaulting to 10"
else
    Egamma_end=$4
fi

for (( i=1; i<=$NumFiles; i++ ))
do
    FileNamesStr+="PairSpecSim_${i}_${NumEvents}_${Egamma_start}_${Egamma_end}/EICReconOut_${i}_${NumEvents}.root "
done

CombinedFile="EICReconOut_Combined_1_${NumFiles}_${NumEvents}.root"
if [ ! -f "${CombinedFile}" ]; then
    hadd ${CombinedFile} ${FileNamesStr}
elif [ -f "${CombinedFile}" ]; then
    read -p "${CombinedFile} already found, remove and remake? <Y/N> " prompt3 
    if [[ $prompt3 == "y" || $prompt3 == "Y" || $prompt3 == "yes" || $prompt3 == "Yes" ]]; then
	rm "${CombinedFile}"
	hadd ${CombinedFile} ${FileNamesStr}
    else echo "Not removing and remaking, all done then!"
    fi
fi

exit 0


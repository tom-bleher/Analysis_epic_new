#!/bin/bash
# 01/06/23 - Stephen Kay, University of York
# Script to combine output after it runs into a single EICRecon file
# Run this from the directory containing all of the output folders (i.e. within /volatile or /cache or wherever the jobs were outputting to
# 07/06/23 - This version (2) is intended for use on files that have been run over a range of Egamma values, it combines *all* of the files across the processed eenergy range

# This is the number of files per beam energy
NumFiles=$1
if [[ -z "$1" ]]; then
    echo "I need to know the number of files per beam energy that were processed so I can combine them!"
    echo "Please provide a number of files per beam energy as the first argument."
    exit 1
fi

NumEvents=$2
if [[ -z "$2" ]]; then
    echo "I need to know the number of events that were processed for each file so I can combine them!"
    echo "Please provide a number of events per file as the second argument."
    exit 2
fi

# Check if an argument was provided for Egamma_start, if not, set 10
re='^[0-9]+$'
if [[ -z "$3" ]]; then
    Egamma_start=10
    echo "Egamma_start not specified, defaulting to 10"
else
    Egamma_start=$3
    if ! [[ $Egamma_start =~ $re ]] ; then # Check it's an integer
	echo "!!! EGamma_start is not an integer !!!" >&2; exit 4
    fi
    if (( $Egamma_start > 18 )); then # If Egamma start is too high, set it to 18
	Egamma_start=18
    fi	
fi

# Check if an argument was provided for Egamma_end, if not, set 10
if [[ -z "$4" ]]; then
    Egamma_end=10
    echo "Egamma_end not specified, defaulting to 10"
else
    Egamma_end=$4
    if ! [[ $Egamma_end =~ $re ]] ; then # Check it's an integer
	echo "!!! EGamma_end is not an integer !!!" >&2; exit 5
    fi
    if (( $Egamma_end > 18 )); then # If Egamma end is too high, set it to 18
	Egamma_end=18
    fi	
fi

if [[ -z "$5" ]]; then
    Egamma_step="0.5"
    echo "Egamma step size not specified, defaulting to 0.5"
else
    Egamma_step=$5
fi
# Round the step size to 2 dp, if 0.00, set to 0.01 and warn
printf -v Egamma_step "%.2f" $Egamma_step
if [[ $Egamma_step == "0.00" ]];then
    echo; echo "!!!!!";echo "Warning, Egamma step size too low, setting to 0.01 by default"; echo "!!!!"; echo;
    Egamma_step="0.01"
fi

if [[ -z "$6" ]]; then
    Gun="False"
    echo "Gun argument not specified, assuming false and running lumi_particles.cxx"
else
    Gun=$6
fi

# Standardise capitlisation of true/false statement, catch any expected/relevant cases and standardise them
if [[ $Gun == "TRUE" || $Gun == "True" || $Gun == "true" ]]; then
    Gun="True"
elif [[ $Gun == "FALSE" || $Gun == "False" || $Gun == "false" ]]; then
    Gun="False"
fi
# Check gun is either true or false, if not, just set it to false
if [[ $Gun != "True" && $Gun != "False" ]]; then
    Gun="False"
    echo "Gun (arg 6) not supplied as true or false, defaulting to False. Enter True/False to enable/disable gun based event generation."
fi

if (( $Egamma_end < $Egamma_start )); then
    Egamma_end=$Egamma_start
fi

for i in $(seq $Egamma_start $Egamma_step $Egamma_end); 
do
    for (( j=1; j<=$NumFiles; j++ ))
    do
	if [[ $Gun == "True" ]]; then
	    FileNamesStr+="PairSpecSim_${j}_${NumEvents}_Gun_${i/./p}_${i/./p}/EICReconOut_${j}_${NumEvents}.root "
	else
	    FileNamesStr+="PairSpecSim_${j}_${NumEvents}_${i/./p}_${i/./p}/EICReconOut_${j}_${NumEvents}.root "
	fi
    done
done

if [[ $Gun == "True" ]]; then
    CombinedFile="EICReconOut_Combined_1_${NumFiles}_${NumEvents}_Gun_${Egamma_start}_${Egamma_end}_Estep_${Egamma_step/./p}.root"
else
    CombinedFile="EICReconOut_Combined_1_${NumFiles}_${NumEvents}_${Egamma_start}_${Egamma_end}_Estep_${Egamma_step/./p}.root"
fi

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


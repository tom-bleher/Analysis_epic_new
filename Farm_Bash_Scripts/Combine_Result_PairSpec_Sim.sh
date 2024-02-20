#!/bin/bash
# 01/06/23 - Stephen Kay, University of York
# Script to combine output after it runs into a single EICRecon file
# Run this from the directory containing all of the output folders (i.e. within /volatile or /cache or whereever the jobs were outputting to
SimDir="/group/eic/users/${USER}/ePIC"

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

# if [[ -z "$5" ]]; then
#     SpagCal="False"
#     echo "SpagCal argument not specified, assuming false and running homogeneous calorimeter simulation"
# else
#     SpagCal=$5
# fi

# Standardise capitlisation of true/false statement, catch any expected/relevant cases and standardise them
# if [[ $SpagCal == "TRUE" || $SpagCal == "True" || $SpagCal == "true" ]]; then
#     SpagCal="True"
#     # If SpagCal specified, find files by mod/fiber size. If no values specified, grab current values from .xml files and try these
#     # Slightly convoluted sed commands, first sed command gets line, second strips any non numerical characters, 3rd removes trailing white space, fourth removes any leading white space, final replaces any remaining white space with p
#     # If value=2.5mm, first four commands would leave you with 2 5, so final command adds a p in the gap for a readable file name
#     if [[ -z "$6" ]]; then
# 	Mod_Size=$(sed -n 8p ${SimDir}/epic/compact/far_backward/lumi/spec_scifi_cal.xml | sed -e 's/[^0-9]/ /g' | sed -e 's/^ *//g' | sed -e 's/ *$//g' | sed 's/ /p/g') # Try to get value from xml file if not specified by user
#     else
# 	Mod_Size=$6
#     fi
#     if [[ -z "$7" ]]; then
# 	Fiber_Size=$(sed -n 7p ${SimDir}/epic/compact/far_backward/lumi/spec_scifi_cal.xml | sed -e 's/[^0-9]/ /g' | sed -e 's/^ *//g' | sed -e 's/ *$//g' | sed 's/ /p/g')
#     else
# 	Fiber_Size=$7
#     fi
# elif [[ $SpagCal == "FALSE" || $SpagCal == "False" || $SpagCal == "false" ]]; then
#     SpagCal="False"
# fi
# # Check gun is either true or false, if not, just set it to false
# if [[ $SpagCal != "True" && $SpagCal != "False" ]]; then
#     SpagCal="False"
#     echo "SpagCal (arg 5) not supplied as true or false, defaulting to False. Enter True/False to enable/disable gun based event generation."
# fi

for (( i=1; i<=$NumFiles; i++ ))
do
    #if [[ $SpagCal == "True" ]]; then
	#FileNamesStr+="PairSpecSim_SpagCal_${Fiber_Size}mmFiber_${Mod_Size}mmMod_${i}_${NumEvents}_${Egamma_start}_${Egamma_end}/EICReconOut_${i}_${NumEvents}.root "
	#FileNamesStr+="PairSpecSim_SpagCal_${i}_${NumEvents}_${Egamma_start}_${Egamma_end}/EICReconOut_${i}_${NumEvents}.root "
    #else
	FileNamesStr+="PairSpecSim_${i}_${NumEvents}_${Egamma_start}_${Egamma_end}/EICReconOut_${i}_${NumEvents}.root "
    #fi
done

#if [[ $SpagCal == "True" ]]; then
#    CombinedFile="${NumFiles}_${NumEvents}_${Egamma_start}_${Egamma_end}_SpagCalCirc_${Mod_Size}_${Fiber_Size}.root"
    #CombinedFile="EICReconOut_SpagCal_Combined_1_${NumFiles}_${NumEvents}.root"
#else
CombinedFile="EICReconOut_Combined_1_${NumFiles}_${NumEvents}.root"
#fi

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


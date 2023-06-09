#! /bin/bash

### Stephen Kay, University of York
### 26/05/23
### stephen.kay@york.ac.uk
### A script to execute an individual simulation for the far backward pair spectrometer
### This version utilises a simple particle gun for the study
### Input args are - FileNum NumEvents Egamma_start (optional) Egamma_end (optional)
### Intended to be fed to some swif2 job submission script

SimDir="/group/eic/users/${USER}/ePIC"
echo "Running as ${USER}"
echo "Assuming simulation directory - ${SimDir}"
if [ ! -d $SimDir ]; then   
    echo "!!! WARNING !!!"
    echo "!!! $SimDir - Does not exist - Double check pathing and try again !!!"
    echo "!!! WARNNING !!!"
    exit 1
fi

FileNum=$1 # First arg is the number of files to run
if [[ -z "$1" ]]; then
    echo "I need a number of files to run!"
    echo "Please provide a number of files to run as the first argument"
    exit 2
fi
NumEvents=$2 # Second argument is an output file name
if [[ -z "$2" ]]; then
    echo "I need a number of events to generate per file!"
    echo "Please provide a number of event to generate per file as the second argument"
    exit 3
fi

# Check if an argument was provided for Egamma_start, if not, set 10
if [[ -z "$3" ]]; then
    Egamma_start=10
    echo "Egamma_start not specified, defaulting to 10"
else
    Egamma_start=$3
fi

# Change output path as desired
OutputPath="/volatile/eic/${USER}/FarBackward_Det_Sim"
export Output_tmp="$OutputPath/PairSpecSim_${FileNum}_${NumEvents}_Gun_${Egamma_start}_${Egamma_start}"
if [ ! -d "${Output_tmp}" ]; then # Add this in this script too so it can be run interactively
    mkdir $Output_tmp
else
    if [ "$(ls -A $Output_tmp)" ]; then # If directory is NOT empty, prompt a warning
	if [[ "${HOSTNAME}" == *"ifarm"* ]]; then # Only print this warning if running interactively
	    echo "!!!!! Warning, ${Output_tmp} directory exists and is not empty! Files may be overwritten! !!!!!"
	fi
    fi
fi
SimOutput="${Output_tmp}\/ddsimOut_${FileNum}_${NumEvents}.edm4hep.root"

export EICSHELL=${SimDir}/eic-shell
# Run EIC shell, copy the gun steering file, edit it, run it with simulation, run reconstruction
# Note, in future, could add additional modification lines to the gun steering file
# E.g. customise vertices etc
# Note, no obvious way to reduce screen dump from eicreon, so piping output to dev/null for now
cat <<EOF | $EICSHELL
cd $SimDir
echo $SimOutput
source Init_Env.sh
echo; echo; echo "Copying and editing gun steering file."; echo; echo;

cp $SimDir/ePIC_PairSpec_Sim/simulations/steeringGun.py ${Output_tmp}
sed -i "18s/.*/SIM.numberOfEvents = "${NumEvents}"/" ${Output_tmp}/steeringGun.py
sed -i '20s%Output.edm4hep.root%'${SimOutput}'%' ${Output_tmp}/steeringGun.py
sed -i "25s/.*/SIM.printLevel = 4/" ${Output_tmp}/steeringGun.py
sed -i '180s/10/'${Egamma_start}'/' ${Output_tmp}/steeringGun.py
sed -i '190s/10000/'${Egamma_start}'000/' ${Output_tmp}/steeringGun.py
sleep 5

echo; echo; echo "Gun steering file copied and edited, running simulation."; echo; echo;

ddsim -v 4 --steeringFile ${Output_tmp}/steeringGun.py --compactFile ${SimDir}/epic/epic_ip6_FB.xml
sleep 5

echo; echo; echo "Simulation finished, running reconstruction."; echo; echo;
cd $Output_tmp
eicrecon -Pplugins=LUMISPECCAL,analyzeLumiHits -PanalyzeLumiHits:Egen=${Egamma_start} -Pjana:nevents=${NumEvents} -PLUMISPECCAL:ECalLumiSpecIslandProtoClusers:splitCluster=1 -PEcalLumiSpecIslandProtoClusters:LogLevel=debug ${Output_tmp}/ddsimOut_${FileNum}_${NumEvents}.edm4hep.root > /dev/null
sleep 30

mv ${Output_tmp}/eicrecon.root ${Output_tmp}/EICReconOut_${FileNum}_${NumEvents}.root

echo; echo; echo "Reconstruction finished, output file is - ${Output_tmp}/EICReconOut_${FileNum}_${NumEvents}.root"; echo; echo;
EOF

exit 0

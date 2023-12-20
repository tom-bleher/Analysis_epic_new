#! /bin/bash

### Stephen Kay, University of York
### 26/05/23
### stephen.kay@york.ac.uk
### A script to execute an individual simulation for the far backward pair spectrometer
### Input args are - FileNum NumEvents Egamma_start (optional) Egamma_end (optional) SpagCal (optional)
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

# Check if an argument was provided for Egamma_end, if not, set 10
if [[ -z "$4" ]]; then
    Egamma_end=10
    echo "Egamma_end not specified, defaulting to 10"
else
    Egamma_end=$4
fi

# Change output path as desired
OutputPath="/volatile/eic/${USER}/FarBackward_Det_Sim"
export Output_tmp="$OutputPath/PairSpecSim_${FileNum}_${NumEvents}_${Egamma_start/./p}_${Egamma_end/./p}"
if [ ! -d "${Output_tmp}" ]; then # Add this in this script too so it can be run interactively
    mkdir $Output_tmp
else
    if [ "$(ls -A $Output_tmp)" ]; then # If directory is NOT empty, prompt a warning
	if [[ "${HOSTNAME}" == *"ifarm"* ]]; then # Only print this warning if running interactively
	    echo "!!!!! Warning, ${Output_tmp} directory exists and is not empty! Files may be overwritten! !!!!!"
	fi
    fi
fi

export EICSHELL=${SimDir}/eic-shell
cd ${SimDir}
# Run EIC shell, generate the events, afterburn them, run the simulation, reconstruct the events
# Note, no obvious way to reduce screen dump from eicreon, so piping output to dev/null for now
cat <<EOF | $EICSHELL

source Init_Env.sh
echo; echo; echo "Generating events."; echo; echo;
cd ${SimDir}/ePIC_PairSpec_Sim/simulations/
if (( $(echo "$Egamma_start == $Egamma_end" | bc -l) )); then
echo "Egamma_start (${Egamma_start}) = Egamma_end (${Egamma_end}), running flat distribution"
root -l -b -q 'lumi_particles.cxx(${NumEvents}, true, false, false, ${Egamma_start}, ${Egamma_end},"${Output_tmp}/genParticles_PhotonsAtIP_${FileNum}_${NumEvents}.hepmc")'
else
echo "Egamma_start (${Egamma_start}) != Egamma_end (${Egamma_end}), running BH distribution"
root -l -b -q 'lumi_particles.cxx(${NumEvents}, false, false, false, ${Egamma_start}, ${Egamma_end},"${Output_tmp}/genParticles_PhotonsAtIP_${FileNum}_${NumEvents}.hepmc")'
fi
sleep 5
abconv ${Output_tmp}/genParticles_PhotonsAtIP_${FileNum}_${NumEvents}.hepmc --plot-off -o ${Output_tmp}/abParticles_PhotonsAtIP_${FileNum}_${NumEvents}
echo; echo; echo "Events generated and afterburned, propagating and converting."; echo; echo;
sleep 5

root -b -l -q 'PropagateAndConvert.cxx("${Output_tmp}/genParticles_PhotonsAtIP_${FileNum}_${NumEvents}.hepmc", "${Output_tmp}/genParticles_electrons_${FileNum}_${NumEvents}.hepmc", -58000)'
sleep 5
root -b -l -q 'PropagateAndConvert.cxx("${Output_tmp}/abParticles_PhotonsAtIP_${FileNum}_${NumEvents}.hepmc", "${Output_tmp}/abParticles_electrons_${FileNum}_${NumEvents}.hepmc", -58000)'
sleep 5

echo; echo; echo "Events propagated and converted, running simulation."; echo; echo;

ddsim -v 4 --inputFiles ${Output_tmp}/abParticles_electrons_${FileNum}_${NumEvents}.hepmc --outputFile ${Output_tmp}/ddsimOut_${FileNum}_${NumEvents}.edm4hep.root --compactFile ${SimDir}/epic/epic_ip6_FB.xml -N ${NumEvents}
sleep 5

echo; echo; echo "Simulation finished, running reconstruction."; echo; echo;
cd $Output_tmp
eicrecon -Pplugins=LUMISPECCAL,analyzeLumiHits -Pjana:nevents=${NumEvents} -PLUMISPECCAL:ECalLumiSpecIslandProtoClusers:splitCluster=1 -PEcalLumiSpecIslandProtoClusters:LogLevel=warn ${Output_tmp}/ddsimOut_${FileNum}_${NumEvents}.edm4hep.root 
sleep 30

mv ${Output_tmp}/eicrecon.root ${Output_tmp}/EICReconOut_${FileNum}_${NumEvents}.root
echo; echo; echo "Reconstruction finished, output file is - ${Output_tmp}/EICReconOut_${FileNum}_${NumEvents}.root"; echo; echo;

EOF

exit 0

#  > /dev/null
# Positions of FB components for propagate and convert fn
#  double ConvStart   = -64499.5;
#  double ConvEnd     = -64500.5;
#  double SweeperEnd  = -63270;
#  double ConvMiddle     = -58000;
#  double AnalyzerStart  = -59400;

# Original set of event generation commands
# cd $SimDir/ePIC_PairSpec_Sim/simulations/
# if (( ${Egamma_start} == ${Egamma_end} )); then
# echo "Egamma_start (${Egamma_start}) = Egamma_end (${Egamma_end}), running flat distribution"
# root -l -b -q 'lumi_particles.cxx(${NumEvents}, true, true, true, ${Egamma_start}, ${Egamma_end},"${Output_tmp}/genParticles_${FileNum}_${NumEvents}.hepmc")'
# else
# echo "Egamma_start (${Egamma_start}) != Egamma_end (${Egamma_end}), running BH distribution"
# root -l -b -q 'lumi_particles.cxx(${NumEvents}, false, true, true, ${Egamma_start}, ${Egamma_end},"${Output_tmp}/genParticles_${FileNum}_${NumEvents}.hepmc")'
# fi
# sleep 5
# abconv ${Output_tmp}/genParticles_${FileNum}_${NumEvents}.hepmc -o ${Output_tmp}/abParticles_${FileNum}_${NumEvents}
# sleep 5

#ddsim -v 4 --inputFiles ${Output_tmp}/abParticles_electrons_${FileNum}_${NumEvents}.hepmc --outputFile ${Output_tmp}/ddsimOut_${FileNum}_${NumEvents}.edm4hep.root --compactFile ${SimDir}/epic/epic_ip6_FB.xml -N ${NumEvents}
#sleep 5

# Run Aranya's new design command
# ddsim -v 4 --inputFiles ${Output_tmp}/abParticles_electrons_${FileNum}_${NumEvents}.hepmc --outputFile ${Output_tmp}/ddsimOut_${FileNum}_${NumEvents}.edm4hep.root --compactFile ${SimDir}/epic_Aranya_PairSpec/epic_ip6_FB.xml -N ${NumEvents}
# sleep 5

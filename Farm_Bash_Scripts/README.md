# Introduction

- This folder contains a bunch of scripts that can be utilised on the JLab farm to process large numbers of files quickly
- The scripts are, broadly speaking, split into two categories
  - Scripts that create and submit jobs
  - Scripts that are actually run in these jobs


# Warnings/Notes - Please read before trying to run anything!

- Note that you do NOT need to be in EIC_Shell to run these scripts, but it must be available
- The expected path is 
  - /group/eic/users/${USER}/ePIC/eic-shell
- Note that the scripts also make use of "Init_Env.sh"
  - Copy this script to /group/eic/users/${USER}/ePIC/ or your equivalent!
- My (SKay) repository is named slightly differently
  - To switch it back to "Analysis_epic", just run (from this directory)
    - sed -i "s%ePIC_PairSpec_Sim%Analysis_epic%" *.sh 

# Script Descriptions

- Please pay attention to the printouts from the scripts themselves!
- Pair_Spec_Sim.sh
	- Produce and run a set of simulations, 4 inputs
	  - Number of files
	  - Number of events per file
	  - Egamma_start
	  - Egamma_end
- Pair_Spec_Sim_v2.sh
  - Similar to the previous version, but with a subtle difference
    - Produces X files of Y event PER BEAM ENERGY in range from Egamma_start to Egamma_end (incremented in steps of 1)
  - Now has 5 arguments
    	  - Number of files per beam energy
	  - Number of events per file
	  - Egamma_start
	  - Egamma_end
	  - Gun - True or False, run with a particle gun steering file or not
- Pair_Spec_Sim_Job.sh
  - Script executed as a farm job, can run interactively
  - Generates events
  - Afterburns events
  - Processes event through simulation
  - Runs simulation output through reconstruction
  - 4 arguments
    	  - File number
	  - Number of events to process
	  - Egamma_start
	  - Egamma_end
- Pair_Spec_Sim_Job_Gun.sh
  - Script executed as a farm job, can run interactively
  - Creates a steering file with a particle gun, settings depend upon input args
  - Generates events
  - Processes event through simulation
  - Runs simulation output through reconstruction
  - 4 arguments
    	  - File number
	  - Number of events to process
	  - Egamma_start
- Combine_Results_PairSpec_Sim.sh
  - Script to combine output after it has run into a single EICRecon file
  - Run this from the directory containing all of your output folders
  - The output is named based upon input arguments
  - 4 arguments
    	  - Number of files
	  - Number of events per file
	  - Egamma_start
	  - Egamma_end
- Combine_Results_PairSpec_Sim_v2.sh
  - Script to combine output after it has run into a single EICRecon file
  - Run this from the directory containing all of your output folders
  - 5 arguments
    	  - Number of files per beam energy
	  - Number of events per file
	  - Egamma_start
	  - Egamma_end
	  - Gun - True or false

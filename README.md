
## Program Overview
Forked from Dhevan, Aranya and Stephen's [repository](https://github.com/dhevang/Analysis_epic), it extends the functionality of the original program to parelilize the ddsim and reconstruction processes for testing different ePIC configurations. The main program is `epic_sim2.py`, which is a modified version of the original `createSimFiles.py`. Before running the program, you need to generate the hepmc files using `createGenFiles.py`.

## Inputs 
- JSON configuration file (`simulation_settings.json`) containing simulation parameters
- ePIC detector configuration files (XML)
- Input hepmc files
- Singularity container image (.sif file)

## Flow
- Loads and validates simulation settings from JSON file
- Sets up logging infrastructure and backup directories
- Prepares detector configurations 
- Executes simulations in parallel using ProcessPoolExecutor
    - Enter `eic-shell` singularity container in subprocess
    - Source the according detector configuration file
    - Run ddsim command
    - Export EICrecon_MY plugin directory 
    - Run `eicrecon` command [Optional]
- Optionally runs reconstruction tasks and merges outputs

## Outputs  
- Simulated event files (ROOT files)
- Reconstruction output files (if enabled)
- Log files for each process
- README file with simulation parameters and settings

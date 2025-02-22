## Program Overview
This project is forked from Dhevan, Aranya, and Stephen's [repository](https://github.com/dhevang/Analysis_epic), and extends the original program's functionality by parallelizing the `ddsim` and reconstruction processes to test different ePIC configurations. The main program is `epic_sim2.py`, which is a modified version of the original `createSimFiles.py` and `createGenFiles.py`.

## Prerequisites
- The `eic-shell` Singularity container must be installed and configured.
- The [`ePIC`](https://github.com/eic/epic) detector must be installed and configured.
- If using the `analyzeSimFiles` plugin (or any other plugin), compile it first. To compile the plugin, head to `EICreconPlugins/analyzeLumiHits/build` and run `cmake .. && make -j$(nproc)`. Verify the installation by exporting the plugin folder (`export EICrecon_MY=${PWD}/EICrecon_MY`), ensuring that the `analyzeSimFiles.sh` file is located in the `EICrecon_MY/plugins` directory. If it's missing, copy the `analyzeLumiHits.so` file from the `EICreconPlugins/analyzeLumiHits/build` directory to `EICrecon_MY/plugins`. Finally, run the `eicrecon -Pplugins=analyzeSimFiles,JTest -Pjana:nevents=10` test command as described in the [tutorial](https://eic.github.io/tutorial-jana2/).

## Running the Program
- Configure the `simulation_settings.json` file with the correct paths and desired parameters.
- Run `epic_sim2.py` **outside** the Singularity container.

## Inputs
- A JSON configuration file (`simulation_settings.json`) containing simulation parameters.
- ePIC detector configuration files (in XML format).
- `.hepmc` files generated by program based on the settings.
- Singularity container image (`.sif` file).

## Program Workflow
- Loads and validates simulation settings from the JSON configuration file.
- Sets up logging and backup directories.
- Prepares the detector configurations.
- Executes simulations in parallel using `ProcessPoolExecutor`:
  - Enters the `eic-shell` Singularity container in each subprocess.
  - Sources the relevant detector configuration file.
  - Runs the `ddsim` command.
  - Exports the `EICrecon_MY` plugin directory.
  - Optionally runs the `eicrecon` command for reconstruction.

## Outputs
- Simulated event files (ROOT files).
- Reconstruction output files (ROOT files, if enabled).
- Log files for each process.
- README file containing simulation parameters and settings.

### Example Output (with reconstruction enabled):
```
├── 2.0x0.1px
│   ├── epic
│   ├── logs
│   ├── output_beamEffectsElectrons_20edm4hep.root
│   ├── output_beamEffectsElectrons_30edm4hep.root
│   ├── output_idealElectrons_20edm4hep.root
│   ├── output_idealElectrons_30edm4hep.root
│   ├── podio_output.root
│   └── recon
│       ├── recon_output_beamEffectsElectrons_20edm4hep.root
│       ├── recon_output_beamEffectsElectrons_30edm4hep.root
│       ├── recon_output_idealElectrons_20edm4hep.root
│       └── recon_output_idealElectrons_30edm4hep.root
├── execution_report.txt
├── overview.log
└── README.txt
```

## More Resources
- [ePIC Tutorials](https://eic.github.io/documentation/tutorials.html)
- [EIC Software Tutorial (unofficial)](https://github.com/JeffersonLab/eic-sftware-tutorial)
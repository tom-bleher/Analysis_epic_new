# Epic Simulation Framework

This project provides a flexible framework for running particle physics simulations with the ePIC detector, with special focus on luminosity spectrometer studies. Originally forked from Dhevan, Aranya, and Stephen's [repository](https://github.com/dhevang/Analysis_epic), it extends the functionality by parallelizing simulations and adding environment-aware execution capabilities.

## Prerequisites

- **Detector**: The [`ePIC`](https://github.com/eic/epic) detector must be installed and configured
- **Singularity**: The `eic-shell` Singularity container (when running outside container)
- **Plugins**: Required reconstruction plugins (if using reconstruction)

### Plugin Setup

If using reconstruction, compile the necessary plugins:

1. Navigate to the plugin directory:
   ```bash
   cd EICreconPlugins/analyzeLumiHits/build
   ```

2. Compile the plugin:
   ```bash
   cmake .. && make -j$(nproc)
   ```

3. Set up the environment:
   ```bash
   export EICrecon_MY=${PWD}/EICrecon_MY
   ```

4. Verify installation:
   ```bash
   # Check that plugin files exist
   ls -l ${EICrecon_MY}/plugins/analyzeLumiHits.so
   # Test the plugin
   eicrecon -Pplugins=analyzeSimFiles,JTest -Pjana:nevents=10
   ```

## Configuration

Edit `simulation_settings.json` to customize your simulation:

```json
{
    "pixel_pairs": [[2.0,0.1], [1.0,0.1]],   // Pixel dimensions to test [dx,dy]
    "particle_count": 5000,                   // Number of particles per simulation
    "simulation_types": ["beamEffectsElectrons", "idealElectrons"],  // Simulation types
    "energy_levels": [20, 30],                // Beam energies to simulate (GeV)
    
    "detector_path": "/path/to/epic",         // Path to ePIC detector installation
    "hepmc_input_path": "/path/to/hepmc",     // Path for HepMC input/output files
    "singularity_image_path": "/path/to/sif", // Path to Singularity image (optional when inside eic-shell)
    "eicrecon_plugin_path": "/path/to/plugins", // Path to reconstruction plugins
    
    "enable_reconstruction": true,            // Whether to run reconstruction
    "enable_console_logging": true            // Enable console output
}
```

## Workflow

1. **Environment Detection**: Determines if running inside or outside Singularity
2. **Configuration Loading**: Reads and validates settings from JSON
3. **Input Generation**: Creates missing HepMC files if needed
4. **Detector Preparation**:
   - Creates copies of the detector for each pixel configuration
   - Modifies XML files to set pixel sizes
   - Compiles each detector variant
5. **Task Generation**: Creates simulation and reconstruction commands
6. **Parallel Execution**:
   - Runs simulation tasks with thread pool
   - Runs reconstruction tasks for successful simulations
7. **Reporting**: Generates execution report and logs

## Output Directory Structure

```
├── 2.0x0.1px/                      # Folder for each pixel configuration
│   ├── epic/                       # Modified detector copy
│   ├── logs/                       # Per-task log files
│   │   ├── beamEffectsElectrons_20_sim_subprocess.log
│   │   └── beamEffectsElectrons_20_recon_subprocess.log
│   ├── output_beamEffectsElectrons_20edm4hep.root  # Simulation output
│   └── recon/                      # Reconstruction outputs
│       └── recon_output_beamEffectsElectrons_20edm4hep.root
├── execution_report.txt            # Summary of execution results
├── overview.log                    # Main log file
└── README.txt                      # Generated info about the run
```

## More Resources

- [ePIC Tutorials](https://eic.github.io/documentation/tutorials.html)
- [EIC Software Tutorial](https://github.com/JeffersonLab/eic-sftware-tutorial)
- [DD4hep Documentation](https://dd4hep.web.cern.ch/dd4hep/)
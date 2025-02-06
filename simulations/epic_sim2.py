"""
Created on Fri Sep  6 15:33:11 2024

@author: tombleher
"""
import os
import shutil
from datetime import datetime
import re
import json
import xml.etree.ElementTree as ET
import subprocess
import logging
from typing import Dict, List, Tuple
from concurrent.futures import ProcessPoolExecutor, as_completed
import time 
import tempfile
from concurrent.futures import ThreadPoolExecutor

class HandleSim(object):
    """
    Handles particle accelerator simulation using ddsim and eicrecon commands.
    
    This class manages the simulation pipeline:
    1. Creates detector configurations based on pixel pair settings
    2. Runs ddsim simulations in Singularity containers
    3. Optionally runs eicrecon reconstruction on simulation outputs
    """

    def __init__(self) -> None:
        # Configuration paths
        self.settings_path: str = "simulation_settings.json"
        self.execution_path: str = os.getcwd()
        self.backup_path: str = ""  # Will be set in init_logger
        self.overview_log_path: str = None  # Will be set in init_logger
        self.detector_path: str = ""  # Instead of det_path
        self.hepmc_input_path: str = ""  # Instead of hepmc_path
        self.singularity_image_path: str = ""  # Instead of sif_path
        self.eicrecon_plugin_path: str = ""  # Instead of plugin_path
        self.simulation_output_path: str = ""  # Instead of sim_output_path

        # Simulation parameters
        self.pixel_pairs: List[Tuple[float, float]] = []  # Instead of px_pairs
        self.particle_count: int = 0  # Instead of num_particles
        self.simulation_types: List[str] = []  # Instead of file_types
        self.energy_levels: List[int] = []  # Instead of energies
        
        # Control flags
        self.enable_reconstruction: bool = False  # Instead of reconstruct
        self.enable_console_logging: bool = False  # Instead of console_logging
        
        # init internal variables
        self.sim_dict: Dict[str, Dict[str, str]] = {}
        self.reconstruct = False  
        self.console_logging = False 
        self.sif_path: str = ""  # Initialize sif_path
        self.plugin_path: str = ""  # Initialize plugin_path
        
        # Set up logger first without using printlog
        self.logger = None  # Initialize logger as None
        
        # Add validation for required paths
        self.required_paths = [
            'detector_path',
            'hepmc_input_path',
            'singularity_image_path'  # Add explicit check for singularity_image_path
        ]
        
        # Add explicit validation for simulation settings
        self.required_simulation_settings = [
            'pixel_pairs',
            'particle_count',
            'detector_path',
            'simulation_types',
            'hepmc_input_path',
            'singularity_image_path'
        ]
        
        # Add validation for required paths when reconstruction is enabled
        self.reconstruct_required_paths = [
            'eicrecon_plugin_path'  # Add eicrecon_plugin_path as required when reconstruction is enabled
        ]
        
        # load settings from JSON
        self.load_settings()
        
        # Now initialize logging after settings are loaded
        self.init_logger()
        
        # Log initialization complete
        self.printlog("Initialized HandleSim class.", level="info")

    def init_logger(self) -> None:
        """
        Initialize the logger after settings are loaded, with improved formatting.
        """
        self.logger = logging.getLogger("main_logger")
        self.logger.setLevel(logging.DEBUG)
        
        # Clear any existing handlers
        self.logger.handlers = []
        
        # Create custom formatter without color codes
        formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
        
        # Create directory structure for log file
        log_dir = os.path.join(self.execution_path, "simEvents", 
                              datetime.now().strftime("%Y%m%d_%H%M%S"))
        os.makedirs(log_dir, exist_ok=True)
        
        # Add file handler - now using backup_path
        self.overview_log_path = os.path.join(log_dir, "overview.log")
        file_handler = logging.FileHandler(self.overview_log_path, mode="w")
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(formatter)
        self.logger.addHandler(file_handler)
        
        # Store the backup path for later use
        self.backup_path = log_dir
        
        # Add console handler if enabled, with plain formatting
        if self.enable_console_logging:  # Use the new variable name
            console_handler = logging.StreamHandler()
            console_handler.setLevel(logging.DEBUG)
            console_handler.setFormatter(formatter)
            self.logger.addHandler(console_handler)
            print("Console logging enabled.")  # Use print since logger isn't ready yet

    def load_settings(self) -> None:
        """
        Load and validate settings from JSON file with enhanced error checking.
        """
        print("Loading settings from JSON file.")  # Debug print statement
        try:
            if not os.path.exists(self.settings_path):
                raise FileNotFoundError(f"Settings file not found at {self.settings_path}")

            with open(self.settings_path, 'r') as file:
                self.settings_dict = json.load(file)
            print(f"Loaded settings from {self.settings_path}.")  # Debug print statement

            # Validate all required settings are present
            missing_settings = [
                setting for setting in self.required_simulation_settings 
                if setting not in self.settings_dict
            ]
            if (missing_settings):
                raise ValueError(f"Missing required settings: {', '.join(missing_settings)}")

            # Validate pixel_pairs format
            if not isinstance(self.settings_dict['pixel_pairs'], list) or \
               not all(isinstance(pair, list) and len(pair) == 2 for pair in self.settings_dict['pixel_pairs']):
                raise ValueError("pixel_pairs must be a list of [dx, dy] pairs")

            # Load initial settings
            self.console_logging = self.settings_dict.get("console_logging", self.console_logging)
            print(f"console_logging set to: {self.console_logging}")  # Debug print statement

            # Load required settings
            required_keys = ["pixel_pairs", "particle_count", "detector_path", "simulation_types", "hepmc_input_path", "enable_reconstruction", "singularity_image_path"]
            for key in required_keys:
                if key not in self.settings_dict or self.settings_dict[key] is None:
                    print(f"Missing or empty key: {key} in settings.")  # Debug print statement
                    raise ValueError(f"Missing or empty key: {key} in settings: {self.settings_dict.get(key)}")

            # Load energies from settings
            self.energy_levels = self.settings_dict.get('energy_levels', [])
            if not self.energy_levels:
                raise ValueError("No energy levels specified in settings")

            # set attributes dynamically
            for key, value in self.settings_dict.items():
                setattr(self, key, value)
                print(f"Set attribute {key} to {value}.")  # Debug print statement

            # Validate all required paths exist
            for path_key in self.required_paths:
                path_value = self.settings_dict.get(path_key)
                if not path_value or not os.path.exists(path_value):
                    raise ValueError(f"Required path '{path_key}' is missing or invalid: {path_value}")
            # Validate singularity_image_path
            self.singularity_image_path = self.settings_dict.get("singularity_image_path")
            if not self.singularity_image_path or not os.path.exists(self.singularity_image_path):
                raise ValueError(f"Singularity image path 'singularity_image_path' is missing or invalid: {self.singularity_image_path}")

            # Add eicrecon_plugin_path to required settings if reconstruction is enabled
            if self.settings_dict.get("enable_reconstruction", False):
                self.required_simulation_settings.append("eicrecon_plugin_path")

            # Additional validation for reconstruction-specific settings
            if self.settings_dict.get("enable_reconstruction", False):
                # Validate eicrecon_plugin_path exists if reconstruction is enabled
                eicrecon_plugin_path = self.settings_dict.get("eicrecon_plugin_path")
                if not eicrecon_plugin_path:
                    raise ValueError("eicrecon_plugin_path is required when enable_reconstruction is enabled")
                
                # Check if eicrecon_plugin_path exists and is accessible
                if not os.path.exists(eicrecon_plugin_path):
                    raise FileNotFoundError(f"eicrecon_plugin_path does not exist: {eicrecon_plugin_path}")
                if not os.path.isdir(eicrecon_plugin_path):
                    raise NotADirectoryError(f"eicrecon_plugin_path is not a directory: {eicrecon_plugin_path}")
                
                # Check if EICrecon_MY directory exists in eicrecon_plugin_path
                eicrecon_my_path = os.path.join(eicrecon_plugin_path, "EICrecon_MY")
                if not os.path.exists(eicrecon_my_path):
                    raise FileNotFoundError(f"EICrecon_MY directory not found in eicrecon_plugin_path: {eicrecon_my_path}")

                # Add to required paths for validation
                self.required_paths.extend(self.reconstruct_required_paths)
                print(f"Added reconstruction-specific required paths: {self.reconstruct_required_paths}")

            # Validate all required paths exist
            for path_key in self.required_paths:
                path_value = self.settings_dict.get(path_key)
                if not path_value or not os.path.exists(path_value):
                    raise ValueError(f"Required path '{path_key}' is missing or invalid: {path_value}")

        except FileNotFoundError:
            print("Failed to load settings, creating default configuration.")  # Debug print statement
            with open(self.settings_path, 'w') as file:
                json.dump(self.def_set_dict, file, indent=4)
            raise RuntimeError(f"Settings JSON created at {self.settings_path}. Edit and rerun.")

        except json.JSONDecodeError as e:
            print(f"Failed to parse settings file: {e}. Check for JSON formatting issues.")  # Debug print statement
            raise ValueError("Invalid JSON format in settings file. Fix the file or delete it to recreate.")

        except Exception as e:
            print(f"Unexpected error while loading settings: {e}")  # Debug print statement
            raise RuntimeError("Failed to load settings due to an unexpected error.") from e

    def setup_logger(self, name: str, level=logging.DEBUG) -> logging.Logger:
        """
        Set up a logger with file and conditional console handlers.
        """
        logger = logging.getLogger(name)
        
        # Prevent duplicate handlers
        if not logger.hasHandlers():
            logger.setLevel(logging.DEBUG)  # Capture all log levels

            # file handler - store in execution directory initially
            log_file = os.path.join(self.execution_path, f"overview.log")
            file_handler = logging.FileHandler(log_file, mode="w")
            file_handler.setLevel(logging.DEBUG)
            file_handler.setFormatter(logging.Formatter("%(asctime)s - %(levelname)s - %(message)s"))
            logger.addHandler(file_handler)

            # console handler (conditionally enabled)
            if self.console_logging:
                console_handler = logging.StreamHandler()
                console_handler.setLevel(logging.DEBUG)
                console_handler.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
                logger.addHandler(console_handler)
                self.printlog("Console logging enabled.", level="info")
        
        return logger

    def printlog(self, message: str, level: str = "info") -> None:
        """
        Log a message based on the specified log level.
        :param message: Message to log.
        :param level: Log level ('info', 'warning', 'error', 'debug', etc.).
        """
        level = level.lower()
        log_function = {
            "info": self.logger.info,
            "warning": self.logger.warning,
            "error": self.logger.error,
            "debug": self.logger.debug,
            "critical": self.logger.critical,
        }.get(level, self.logger.info)  # Default to info level if unrecognized

        # Log the message (console output handled by logger configuration)
        log_function(message)

    def init_paths(self) -> None:
        """
        Initialize and validate all required file system paths.
        
        Creates output directories and verifies existence of input paths.
        Raises ValueError if required paths are missing.
        """
        self.printlog("Initializing paths.", level="info")

        # Setup output paths
        if not self.simulation_output_path:
            self.simulation_output_path = os.path.join(self.execution_path, "simEvents")
        os.makedirs(self.simulation_output_path, exist_ok=True)

        # Validate input paths
        self._validate_required_paths()
        
        # Create timestamped backup directory
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.backup_path = os.path.join(self.simulation_output_path, timestamp)
        os.makedirs(self.backup_path, exist_ok=True)

    def _validate_required_paths(self) -> None:
        """Validate existence of all required input paths."""
        required_paths = {
            'detector_path': self.detector_path,
            'hepmc_input_path': self.hepmc_input_path,
            'singularity_image_path': self.singularity_image_path
        }
        
        if self.enable_reconstruction:
            required_paths['eicrecon_plugin_path'] = self.eicrecon_plugin_path
        
        for name, path in required_paths.items():
            if not path or not os.path.exists(path):
                raise ValueError(f"Required path '{name}' is missing or invalid: {path}")

    def get_energies(self) -> None:
        """
        Check for required hepmc files and create them if missing.
        """
        self.printlog("Checking for required hepmc files...", level="info")
        
        # Create results directory if it doesn't exist
        os.makedirs(self.hepmc_input_path, exist_ok=True)
        
        # Check for missing files by looping through all combinations
        missing_files = []
        for energy in self.energy_levels:
            for file_type in self.simulation_types:
                files_to_check = [
                    f"idealPhotonsAtIP_{energy}.hepmc",
                    f"{file_type}_{energy}.hepmc"
                ]
                if file_type == "beamEffectsElectrons":
                    files_to_check.append(f"beamEffectsPhotonsAtIP_{energy}.hepmc")
                
                for hepmc_file in files_to_check:
                    if not os.path.exists(os.path.join(self.hepmc_input_path, hepmc_file)):
                        missing_files.append((energy, file_type))
                        break  # If any file is missing for this energy/type combo, we need to regenerate
        
        if missing_files:
            self.printlog(f"Missing files for energy/type combinations: {missing_files}", level="info")
            self.create_hepmc(missing_files)
        else:
            self.printlog("All required hepmc files found.", level="info")

    def create_hepmc(self, missing_files: List[Tuple[int, str]]) -> None:
        """
        Create missing hepmc files following createGenFiles.py workflow exactly.
        Executes root commands inside singularity container.
        """
        self.printlog("Creating missing hepmc files...", level="info")
        
        # Get location from settings
        location = self.settings_dict.get('location', 'POS.ConvMiddle')
        
        # Create results directory if it doesn't exist
        os.makedirs(self.hepmc_input_path, exist_ok=True)
        
        # Get the directory containing the macro files
        macro_dir = os.path.dirname(os.path.abspath(__file__))
        # Get the project root directory (parent of simulations)
        project_root = os.path.dirname(macro_dir)
        utilities_dir = os.path.join(project_root, "utilities")
        
        # Validate required macro files exist
        required_files = ["lumi_particles.cxx", "PropagateAndConvert.cxx"]
        for file in required_files:
            if not os.path.exists(os.path.join(macro_dir, file)):
                raise FileNotFoundError(f"{file} not found in scripts directory: {macro_dir}")
        
        # Validate constants.h exists
        if not os.path.exists(os.path.join(utilities_dir, "constants.h")):
            raise FileNotFoundError(f"constants.h not found in utilities directory: {utilities_dir}")
        
        # Process each missing energy/type combination
        for energy, _ in set(missing_files):  # Use set to process each energy once
            try:
                self.printlog(f"\nProcessing energy level: {energy} GeV", level="info")
                
                # Step 1: Create ideal photons
                ideal_photons_file = os.path.join(self.hepmc_input_path, f"idealPhotonsAtIP_{energy}.hepmc")
                if not os.path.exists(ideal_photons_file):
                    self.printlog("Generating ideal photons...", level="info")
                    
                    # Create the command to run inside singularity - match createGenFiles.py exactly
                    root_cmd = f"cd {macro_dir} && root -b -q 'lumi_particles.cxx({self.particle_count},true,false,false,{energy},{energy},\"{ideal_photons_file}\")'"
                    
                    # Run command in singularity
                    cmd = [
                        "singularity", "exec", "--containall",
                        "--bind", f"{self.hepmc_input_path}:{self.hepmc_input_path}",
                        "--bind", f"{macro_dir}:{macro_dir}",
                        "--bind", f"{project_root}:{project_root}",
                        "--bind", f"{self.detector_path}:{self.detector_path}",
                        self.singularity_image_path,
                        "/bin/bash", "-c", root_cmd
                    ]
                    
                    result = subprocess.run(cmd, capture_output=True, text=True)
                    if result.returncode != 0:
                        raise RuntimeError(f"Failed to generate ideal photons: {result.stderr}")
                    
                    if not os.path.exists(ideal_photons_file):
                        raise RuntimeError(f"Ideal photons file not created: {ideal_photons_file}")
                    
                    self.printlog(f"Generated ideal photons for {energy} GeV", level="info")
                else:
                    self.printlog(f"Using existing ideal photons file for {energy} GeV", level="info")
                
                # Step 2: Create beam effects version
                beam_effects_file = os.path.join(self.hepmc_input_path, f"beamEffectsPhotonsAtIP_{energy}.hepmc")
                if not os.path.exists(beam_effects_file):
                    self.printlog("Generating beam effects photons...", level="info")
                    
                    # Create the command to run inside singularity - match createGenFiles.py exactly
                    abconv_cmd = f"cd {macro_dir} && abconv {ideal_photons_file} --plot-off -o {os.path.splitext(beam_effects_file)[0]}"
                    
                    # Run command in singularity
                    cmd = [
                        "singularity", "exec", "--containall",
                        "--bind", f"{self.hepmc_input_path}:{self.hepmc_input_path}",
                        "--bind", f"{macro_dir}:{macro_dir}",
                        "--bind", f"{project_root}:{project_root}",
                        "--bind", f"{self.detector_path}:{self.detector_path}",
                        self.singularity_image_path,
                        "/bin/bash", "-c", abconv_cmd
                    ]
                    
                    result = subprocess.run(cmd, capture_output=True, text=True)
                    if result.returncode != 0:
                        raise RuntimeError(f"Failed to generate beam effects photons: {result.stderr}")
                    
                    if not os.path.exists(beam_effects_file):
                        raise RuntimeError(f"Beam effects file not created: {beam_effects_file}")
                    
                    self.printlog(f"Generated beam effects photons for {energy} GeV", level="info")
                else:
                    self.printlog(f"Using existing beam effects file for {energy} GeV", level="info")
                
                # Step 3: Propagate both versions to electrons
                for photon_type in ["ideal", "beamEffects"]:
                    input_file = os.path.join(self.hepmc_input_path, f"{photon_type}PhotonsAtIP_{energy}.hepmc")
                    output_file = os.path.join(self.hepmc_input_path, f"{photon_type}Electrons_{energy}.hepmc")
                    
                    if not os.path.exists(output_file):
                        self.printlog(f"Propagating {photon_type} photons to electrons...", level="info")
                        
                        # Create the command to run inside singularity - match createGenFiles.py exactly
                        prop_cmd = f"cd {macro_dir} && root -b -q 'PropagateAndConvert.cxx(\"{input_file}\",\"{output_file}\",{location})'"
                        
                        # Run command in singularity
                        cmd = [
                            "singularity", "exec", "--containall",
                            "--bind", f"{self.hepmc_input_path}:{self.hepmc_input_path}",
                            "--bind", f"{macro_dir}:{macro_dir}",
                            "--bind", f"{project_root}:{project_root}",
                            "--bind", f"{self.detector_path}:{self.detector_path}",
                            self.singularity_image_path,
                            "/bin/bash", "-c", prop_cmd
                        ]
                        
                        result = subprocess.run(cmd, capture_output=True, text=True)
                        if result.returncode != 0:
                            raise RuntimeError(f"Failed to propagate {photon_type} photons: {result.stderr}")
                        
                        if not os.path.exists(output_file):
                            raise RuntimeError(f"Electron file not created: {output_file}")
                        
                        self.printlog(f"Generated {photon_type} electrons for {energy} GeV", level="info")
                    else:
                        self.printlog(f"Using existing {photon_type} electrons file for {energy} GeV", level="info")
                
                self.printlog(f"Successfully processed energy level {energy} GeV", level="info")
                
            except Exception as e:
                self.printlog(f"Error processing energy {energy}: {str(e)}", level="error")
                raise

    def _run_command_in_singularity(self, cmd: str, work_dir: str) -> None:
        """
        Run a command in Singularity with proper bindings and working directory.
        """
        singularity_cmd = [
            "singularity", "exec", "--containall",
            "--bind", f"{self.hepmc_input_path}:{self.hepmc_input_path}",
            "--bind", f"{work_dir}:{work_dir}",
            self.singularity_image_path,
            "bash", "-c", 
            f"cd {work_dir} && {cmd}"
        ]
        
        result = subprocess.run(singularity_cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"Command failed: {result.stderr}")

    def init_vars(self) -> None:
        """
        Initialize variables and settings from JSON.
        """
        # default if user does not provide JSON
        self.def_set_dict = {
            "pixel_pairs": [[1.0,0.1],[0.1,0.1],[2.0,0.1]],
            "energy_levels": [10,15,20,25,30],
            "particle_count": 100000,
            "detector_path": "/data/tomble/eic/epic",
            "simulation_types": "idealElectrons",
            "hepmc_input_path": "/data/tomble/Analysis_epic_new/simulations/genEvents/results",
            "singularity_image_path": "/data/tomble/eic/local/lib/eic_xl-nightly.sif",
            "eicrecon_plugin_path": "/data/tomble/Analysis_epic_new/EICreconPlugins",
            "enable_reconstruction": True,
            "console_logging": True,
            "simulation_output_path": "",
            "det_ip6_path": "",
            "sim_description": ""
        }

        # check that all boolean settings are correct
        bool_keys = ["enable_reconstruction", "console_logging"]
        for bkey in bool_keys:
            if not hasattr(self, bkey):
                self.printlog(f"Missing key: '{bkey}' in settings.", level="error")
                raise KeyError(f"Missing key: '{bkey}' in settings.")
            
            value = getattr(self, bkey)
            
            if isinstance(value, str):
                value = value.lower() == "true"
            
            if not isinstance(value, bool):
                self.printlog(f"Invalid value for '{bkey}' in settings. Expected a boolean.", level="error")
                raise ValueError(f"Invalid value for '{bkey}' in settings. Expected a boolean.")
            
            setattr(self, bkey, value)
            self.printlog(f"Set boolean attribute {bkey} to {value}.", level="debug")

    def prep_sim(self) -> None:
        """
        Prepare simulation environment following required order:
        1. Loop over pixel pairs
        2. Loop over file types 
        3. Loop over energies
        """
        self.printlog("Preparing simulation.", level="info")
        
        # Validate pixel_pairs before starting
        if not self.pixel_pairs:
            raise ValueError("No pixel pairs defined for simulation")
        
        # Create simulation root directory
        os.makedirs(self.backup_path, exist_ok=True)
        
        # Primary loop: pixel pairs
        for curr_px_dx, curr_px_dy in self.pixel_pairs:
            px_key = f"{curr_px_dx}x{curr_px_dy}"
            curr_sim_path = os.path.join(self.backup_path, f"{px_key}px")
            
            try:
                # 1. Copy detector
                os.makedirs(curr_sim_path, exist_ok=True)
                curr_sim_det_path = self.copy_epic(curr_sim_path)
                
                # 2. Modify detector settings for this pixel pair
                self.mod_detector_settings(curr_sim_det_path, curr_px_dx, curr_px_dy)
                
                # 3. Compile detector after modifications
                self.compile_epic(curr_sim_det_path)
                
                # Secondary loop: file types
                for file_type in self.simulation_types:
                    # Tertiary loop: energies
                    for energy in self.energy_levels:
                        self._setup_simulation_for_config(
                            curr_sim_path,
                            curr_sim_det_path,
                            px_key,
                            file_type,
                            energy
                        )
                
            except Exception as e:
                self.printlog(f"Failed to prepare simulation for {px_key}: {e}", level="error")
                raise

    def _setup_simulation_for_config(self, sim_path: str, det_path: str, px_key: str, 
                                   file_type: str, energy: int) -> None:
        """Setup simulation for a specific configuration."""
        # Verify input file exists
        input_file = os.path.join(self.hepmc_input_path, f"{file_type}_{energy}.hepmc")
        if not os.path.exists(input_file):
            self.printlog(f"Warning: Input file not found: {input_file}", level="warning")
            return

        # Setup paths
        sim_output = os.path.join(sim_path, f"output_{file_type}_{energy}edm4hep.root")
        os.makedirs(os.path.dirname(sim_output), exist_ok=True)

        # Add to simulation dictionary
        if px_key not in self.sim_dict:
            self.sim_dict[px_key] = {
                "sim_det_path": det_path,
                "sim_compact_path": os.path.join(det_path, "install/share/epic/compact"),
                "sim_ip6_path": os.path.join(det_path, "install/share/epic/epic_ip6_extended.xml"),
                "sim_shell_path": os.path.join(det_path, "install/bin/thisepic.sh"),
                "ddsim_cmds": [],
                "recon_cmds": [] if self.enable_reconstruction else None,
                "task_ids": []
            }

        # Generate commands
        task_id = f"{px_key}_{file_type}_{energy}"
        self.sim_dict[px_key]["task_ids"].append(task_id)
        
        # Add ddsim command
        ddsim_cmd = self.get_ddsim_cmd(input_file, sim_output, 
                                      self.sim_dict[px_key]["sim_ip6_path"])
        self.sim_dict[px_key]["ddsim_cmds"].append(ddsim_cmd)

        # Add reconstruction command if enabled
        if self.enable_reconstruction:
            recon_dir = os.path.join(sim_path, "recon")
            os.makedirs(recon_dir, exist_ok=True)
            recon_output = os.path.join(recon_dir, f"recon_output_{file_type}_{energy}edm4hep.root")
            recon_cmd = self.get_recon_cmd(sim_output, recon_output, det_path)
            self.sim_dict[px_key]["recon_cmds"].append(recon_cmd)

    def get_ddsim_cmd(self, input_file: str, output_file: str, compact_file: str) -> str:
        """
        Generate a ddsim command with consistent parameters
        """
        return (
            f"ddsim --inputFiles {input_file} "
            f"--outputFile {output_file} "
            f"--compactFile {compact_file} "
            f"-N {self.particle_count} "
        )

    def get_recon_cmd(self, input_file: str, output_file: str, detector_path: str) -> str:
        """
        Generate a reconstruction command with proper environment setup.
        
        The reconstruction requires:
        1. Source detector environment
        2. Export EICrecon_MY plugin path
        3. Run eicrecon with proper plugin configuration
        """
        # Create plugin directory path
        plugin_dir = os.path.join(self.eicrecon_plugin_path, "EICrecon_MY")
        
        return (
            f"source {os.path.join(detector_path, 'install/bin/thisepic.sh')} && "
            f"export EICrecon_MY={plugin_dir} && "
            f"eicrecon "
            f"-Pplugins=analyzeLumiHits "
            f"-Phistsfile={output_file} {input_file}"
        )

    def compile_epic(self, detector_path: str) -> None:
        """
        Compile a copied detector in its dedicated pixel path.
        """
        self.printlog(f"Compiling detector at {detector_path}", level="info")
        try:
            build_path = os.path.join(detector_path, 'build')
            if os.path.exists(build_path):
                shutil.rmtree(build_path)
            os.makedirs(build_path)
            
            # Run compilation inside Singularity
            cmd = [
                "singularity", "exec", "--containall",
                "--bind", f"{os.path.dirname(detector_path)}:{os.path.dirname(detector_path)}",
                self.singularity_image_path,
                "/bin/bash", "-c", f"""
                    set -e
                    cd {detector_path}
                    mkdir -p build && cd build
                    cmake -DCMAKE_INSTALL_PREFIX=../install ..
                    make -j$(nproc) install
                """
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            self.printlog(f"Compilation output: {result.stdout}", level="debug")
            
            # Verify installation
            install_path = os.path.join(detector_path, 'install')
            if not os.path.exists(install_path):
                raise RuntimeError(f"Installation directory not created: {install_path}")
                
            thisepic_path = os.path.join(install_path, 'bin/thisepic.sh')
            if not os.path.exists(thisepic_path):
                raise RuntimeError(f"thisepic.sh not found at: {thisepic_path}")
                
        except Exception as e:
            self.printlog(f"Failed to compile detector: {e}", level="error")
            raise

    def copy_epic(self, curr_sim_path: str) -> str:
        """
        Copy detector to simulation directory.
        
        Args:
            curr_sim_path (str): Path to current simulation directory
            
        Returns:
            str: Path to the copied detector
        """
        self.printlog(f"Copying epic detector to {curr_sim_path}.", level="info")
        try:
            # Copy detector
            det_name = os.path.basename(self.detector_path)
            dest_path = os.path.join(curr_sim_path, det_name)
            
            # Copy fresh detector
            if os.path.exists(dest_path):
                shutil.rmtree(dest_path)
            shutil.copytree(self.detector_path, dest_path, symlinks=True)
            self.printlog(f"Successfully copied detector to {dest_path}", level="info")
            
            return dest_path
                
        except Exception as e:
            self.printlog(f"Failed to copy detector: {e}", level="error")
            raise

    def mod_detector_settings(
        self, 
        curr_epic_path, 
        curr_px_dx, 
        curr_px_dy
        ) -> None:
        """
        Method for rewriting desired pixel values for all XML files of the Epic 
        detector. For every "{DETECTOR_PATH}" in copied epic XMLs, we replace with the path 
        for the current compact pixel path, and for every compact path 
        we replace with our new compact path 

        Args:
            curr_epic_path (str): Path to the copied detector directory.
            curr_px_dx (float): Pixel size in the X direction (dx).
            curr_px_dy (float): Pixel size in the Y direction (dy).
        """
        self.printlog(f"Modifying detector settings for {curr_px_dx}x{curr_px_dy}.", level="info")
        # iterate over all XML files in the copied epic directory
        for subdir, dirs, files in os.walk(curr_epic_path):
            for file in files:
                filepath = subdir + os.sep + file
                if file.endswith(".xml") and os.access(filepath, os.W_OK):
                    filepath = os.path.join(subdir, file)
                    try:
                        tree = ET.parse(filepath)
                        root = tree.getroot()
                        for elem in root.iter():
                            # update pixel size constants in the XML file
                            if elem.tag == "constant" and 'name' in elem.attrib:
                                if elem.attrib['name'] == "LumiSpecTracker_pixelSize_dx":
                                    elem.set('value', f"{curr_px_dx}*mm")
                                elif elem.attrib['name'] == "LumiSpecTracker_pixelSize_dy":
                                    elem.set('value', f"{curr_px_dy}*mm")
                        tree.write(filepath)
                        self.printlog(f"Updated {filepath} with pixel sizes dx={curr_px_dx}, dy={curr_px_dy}.", level="info")
                    except Exception as e:
                        self.printlog(f"Failed to modify {filepath}: {e}", "error")
                        raise RuntimeError(f"Error in mod_detector_settings: {filepath}") from e

    def execute_task(self, task: dict) -> dict:
        """Execute a single task with improved logging and error handling."""
        task_id = task['task_id']
        px_key = task['px_key']
        cmd = task['cmd']
        task_type = task['type']  # Add task type to distinguish between sim and recon
        
        # Setup logging for this task
        logger, log_file = self.setup_subprocess_logger(cmd, px_key, task_type)
        logger.info(f"Starting {task_type} task {task_id}")
        logger.info(f"Command: {cmd}")
        
        try:
            # Build singularity command with proper bindings
            singularity_cmd = [
                "singularity", "exec", "--containall",
                "--bind", f"{self.detector_path}:{self.detector_path}",
                "--bind", f"{self.execution_path}:{self.execution_path}",
                "--bind", f"{self.hepmc_input_path}:{self.hepmc_input_path}",
                "--bind", f"{self.eicrecon_plugin_path}:{self.eicrecon_plugin_path}",
                self.singularity_image_path,
                "/bin/bash", "-c"
            ]

            # Add source commands for environment setup
            if task_type == 'sim':
                source_cmd = f"source {task['shell_path']} && "
            else:  # reconstruction
                source_cmd = (
                    f"source {task['shell_path']} && "
                    f"export EICrecon_MY={self.eicrecon_plugin_path}/EICrecon_MY && "
                    f"export DETECTOR_PATH={os.path.dirname(task['shell_path'])} && "
                )
                logger.info(f"Setting up reconstruction environment:")
                logger.info(f"Shell path: {task['shell_path']}")
                logger.info(f"EICrecon_MY: {self.eicrecon_plugin_path}/EICrecon_MY")
                logger.info(f"DETECTOR_PATH: {os.path.dirname(task['shell_path'])}")
            
            # Combine commands
            full_cmd = singularity_cmd + [f"{source_cmd}{task['cmd']}"]
            
            # Execute with output capture
            logger.info(f"Executing {task_type} command...")
            result = subprocess.run(
                full_cmd,
                capture_output=True,
                text=True,
                check=True
            )
            
            # Log outputs
            logger.info(f"{task_type.capitalize()} command output:")
            logger.info(result.stdout)
            if result.stderr:
                logger.warning(f"{task_type.capitalize()} command stderr:")
                logger.warning(result.stderr)

            # Verify output file exists for reconstruction
            if task_type == 'recon':
                output_file = self._extract_output_path(cmd)
                if output_file and os.path.exists(output_file):
                    file_size = os.path.getsize(output_file)
                    logger.info(f"Reconstruction output file created: {output_file} ({file_size} bytes)")
                else:
                    logger.error(f"Reconstruction output file not found: {output_file}")
                    raise RuntimeError(f"Reconstruction failed - output file not created")
            
            return {
                'task_id': task_id,
                'status': 'completed',
                'output': result.stdout,
                'error': result.stderr
            }
            
        except subprocess.CalledProcessError as e:
            logger.error(f"{task_type.capitalize()} task failed with exit code {e.returncode}")
            logger.error(f"Error output: {e.stderr}")
            return {
                'task_id': task_id,
                'status': 'failed',
                'error': str(e.stderr)
            }
        except Exception as e:
            logger.error(f"Unexpected error during {task_type}: {str(e)}")
            return {
                'task_id': task_id,
                'status': 'failed',
                'error': str(e)
            }

    def setup_subprocess_logger(self, cmd: str, px_key: str, task_type: str) -> Tuple[logging.Logger, str]:
        """
        Create a separate logger for each subprocess with detailed identification.
        """
        # Initialize log_file at the start
        log_file = None
        
        # Extract file type and energy from command for better logging identification
        file_type = None
        energy = None
        
        # Parse command to get file type and energy
        if task_type == 'sim':
            if '--inputFiles' in cmd:
                input_file = cmd.split('--inputFiles')[1].split()[0]
                base_name = os.path.basename(input_file)
                if base_name.endswith('.hepmc'):
                    parts = base_name.replace('.hepmc', '').split('_')
                    if len(parts) >= 2:
                        file_type = parts[0]
                        energy = parts[1]
        else:  # reconstruction
            if 'recon_output_' in cmd:
                output_parts = [p for p in cmd.split() if 'recon_output_' in p][0]
                parts = os.path.basename(output_parts).split('_')
                if len(parts) >= 3:
                    file_type = parts[1]
                    energy = parts[2].replace('edm4hep.root', '')
        
        # Create unique logger name
        process_id = f"{px_key}_{file_type}_{energy}_{task_type}_pid{os.getpid()}" if file_type and energy else f"{px_key}_{task_type}_pid{os.getpid()}"
        logger_name = f"subprocess_{process_id}"
        logger = logging.getLogger(logger_name)
        
        # Create log file path even if logger already has handlers
        px_path = os.path.join(self.backup_path, f"{px_key}px")
        os.makedirs(px_path, exist_ok=True)
        
        logs_dir = os.path.join(px_path, "logs")
        os.makedirs(logs_dir, exist_ok=True)
        
        if file_type and energy:
            log_file = os.path.join(logs_dir, f"{file_type}_{energy}_{task_type}_subprocess.log")
        else:
            log_file = os.path.join(logs_dir, f"{task_type}_subprocess_{os.getpid()}.log")
        
        if not logger.hasHandlers():
            logger.setLevel(logging.DEBUG)
            
            file_handler = logging.FileHandler(log_file)
            file_handler.setFormatter(logging.Formatter(
                '%(asctime)s - %(name)s - Process %(process)d - %(levelname)s - %(message)s'
            ))
            logger.addHandler(file_handler)
            
            if self.console_logging:
                console_handler = logging.StreamHandler()
                console_handler.setFormatter(logging.Formatter(
                    '%(levelname)s - %(name)s - Process %(process)d - %(message)s'
                ))
                logger.addHandler(console_handler)
        
        return logger, log_file

    def _extract_output_path(self, cmd: str) -> str:
        """Extract the output file path from a reconstruction command."""
        try:
            # Look for -Phistsfile= parameter
            for part in cmd.split():
                if part.startswith('-Phistsfile='):
                    return part.split('=')[1].strip('"')
            return None
        except Exception:
            return None

    def exec_sim(self) -> None:
        """Execute simulation and reconstruction with proper task dependencies."""
        task_status = {}
        max_workers = max(1, os.cpu_count() - 1)
        
        # First run all simulation tasks
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            sim_futures = {}
            for task in self.get_simulation_tasks():
                future = executor.submit(self.execute_task, task)
                sim_futures[future] = task
                
            for future in as_completed(sim_futures):
                task = sim_futures[future]
                try:
                    result = future.result()
                    task_status[result['task_id']] = result
                    self.printlog(f"Simulation {result['task_id']} completed with status: {result['status']}")
                except Exception as e:
                    self.printlog(f"Simulation failed: {str(e)}", level="error")
                    task_status[task['task_id']] = {'status': 'failed', 'error': str(e)}

        # Validate reconstruction setup before running tasks
        if self.enable_reconstruction:
            try:
                self._validate_reconstruction_setup()
            except Exception as e:
                self.printlog(f"Reconstruction validation failed: {e}", level="error")
                return
            
            # Run reconstruction tasks
            with ThreadPoolExecutor(max_workers=max_workers) as executor:
                recon_futures = {}
                for task in self.get_reconstruction_tasks(task_status):
                    future = executor.submit(self.execute_task, task)
                    recon_futures[future] = task
                    
            for future in as_completed(recon_futures):
                task = recon_futures[future]
                try:
                    result = future.result()
                    task_status[result['task_id']] = result
                    self.printlog(f"Reconstruction {result['task_id']} completed with status: {result['status']}")
                except Exception as e:
                    self.printlog(f"Reconstruction failed: {str(e)}", level="error")
                    task_status[task['task_id']] = {'status': 'failed', 'error': str(e)}

        self.create_execution_report(task_status)

    def create_execution_report(self, task_status: dict) -> None:
        """
        Create a detailed execution report with enhanced information.
        """
        report_path = os.path.join(self.backup_path, "execution_report.txt")
        with open(report_path, 'w') as f:
            f.write("EPIC Simulation Execution Report\n")
            f.write("==============================\n")
            f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            # Write simulation settings
            f.write("Simulation Settings\n")
            f.write("-----------------\n")
            f.write(f"Number of particles: {self.particle_count}\n")
            f.write(f"File types: {', '.join(self.simulation_types)}\n")
            f.write(f"Energies: {', '.join(map(str, self.energy_levels))}\n")
            f.write(f"Reconstruction enabled: {self.enable_reconstruction}\n\n")
            
            # Summary statistics
            total = len(task_status)
            completed = sum(1 for status in task_status.values() if status['status'] == 'completed')
            failed = sum(1 for status in task_status.values() if status['status'] == 'failed')
            
            f.write("Overall Statistics\n")
            f.write("-----------------\n")
            f.write(f"Total tasks: {total}\n")
            f.write(f"Completed tasks: {completed}\n")
            f.write(f"Failed tasks: {failed}\n")
            f.write(f"Success rate: {(completed/total)*100:.2f}%\n\n")
            
            # Group tasks by pixel pair
            tasks_by_pixel = {}
            for task_id, status in task_status.items():
                parts = task_id.split('_')
                px_key = f"{parts[0]}x{parts[1]}"
                
                if px_key not in tasks_by_pixel:
                    tasks_by_pixel[px_key] = {
                        'ddsim': {'completed': [], 'failed': []},
                        'recon': {'completed': [], 'failed': []}
                    }
                
                # Determine if this is a ddsim or reconstruction task
                is_recon = 'recon' in task_id
                task_type = 'recon' if is_recon else 'ddsim'
                task_status = status['status']
                
                # Extract file type and energy
                if len(parts) >= 4:
                    file_type = parts[2]
                    energy = parts[3]
                    task_info = {
                        'file_type': file_type,
                        'energy': energy,
                        'error': status.get('error', None)
                    }
                    if task_status == 'completed':
                        tasks_by_pixel[px_key][task_type]['completed'].append(task_info)
                    else:
                        tasks_by_pixel[px_key][task_type]['failed'].append(task_info)
            
            # Write detailed status for each pixel pair
            f.write("\nDetailed Task Status by Pixel Pair\n")
            f.write("================================\n")
            
            for px_key, tasks in tasks_by_pixel.items():
                f.write(f"\nPixel Pair: {px_key}\n")
                f.write("-" * (len(px_key) + 12) + "\n")
                
                # Write ddsim tasks
                f.write("\nSimulation Tasks:\n")
                if tasks['ddsim']['completed']:
                    f.write("  Completed:\n")
                    for task in tasks['ddsim']['completed']:
                        f.write(f"    - {task['file_type']} at {task['energy']} GeV\n")
                if tasks['ddsim']['failed']:
                    f.write("  Failed:\n")
                    for task in tasks['ddsim']['failed']:
                        f.write(f"    - {task['file_type']} at {task['energy']} GeV\n")
                        if task['error']:
                            f.write(f"      Error: {task['error']}\n")
                
                # Write reconstruction tasks
                if self.enable_reconstruction:
                    f.write("\nReconstruction Tasks:\n")
                    if tasks['recon']['completed']:
                        f.write("  Completed:\n")
                        for task in tasks['recon']['completed']:
                            f.write(f"    - {task['file_type']} at {task['energy']} GeV\n")
                    if tasks['recon']['failed']:
                        f.write("  Failed:\n")
                        for task in tasks['recon']['failed']:
                            f.write(f"    - {task['file_type']} at {task['energy']} GeV\n")
                            if task['error']:
                                f.write(f"      Error: {task['error']}\n")
                
                f.write("\n" + "="*50 + "\n")

            # Write timestamp
            f.write(f"\nReport generated at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")

    def get_simulation_tasks(self) -> List[dict]:
        """
        Generate simulation tasks from sim_dict.
        
        Returns:
            List of task dictionaries containing command and metadata
        """
        tasks = []
        for px_key, sim_info in self.sim_dict.items():
            for cmd_idx, cmd in enumerate(sim_info['ddsim_cmds']):
                task_id = sim_info['task_ids'][cmd_idx]
                tasks.append({
                    'task_id': task_id,
                    'px_key': px_key,
                    'type': 'sim',
                    'cmd': cmd,
                    'shell_path': sim_info['sim_shell_path'],
                    'detector_path': sim_info['sim_det_path']
                })
        
        self.printlog(f"Generated {len(tasks)} simulation tasks", level="info")
        return tasks

    def get_reconstruction_tasks(self, task_status: dict) -> List[dict]:
        """
        Generate reconstruction tasks for successful simulations.
        
        Args:
            task_status: Dictionary of completed simulation task statuses
            
        Returns:
            List of reconstruction task dictionaries
        """
        tasks = []
        for px_key, sim_info in self.sim_dict.items():
            if not sim_info['recon_cmds']:
                continue
            
            for cmd_idx, cmd in enumerate(sim_info['recon_cmds']):
                sim_task_id = sim_info['task_ids'][cmd_idx]
                
                # Only create reconstruction task if simulation was successful
                sim_status = task_status.get(sim_task_id, {}).get('status')
                if sim_status == 'completed':
                    task_id = f"recon_{sim_task_id}"
                    tasks.append({
                        'task_id': task_id,
                        'px_key': px_key,
                        'type': 'recon',
                        'cmd': cmd,
                        'shell_path': sim_info['sim_shell_path'],
                        'detector_path': sim_info['sim_det_path']
                    })
                else:
                    self.printlog(
                        f"Skipping reconstruction for {sim_task_id} due to simulation status: {sim_status}", 
                        level="warning"
                    )
        
        self.printlog(f"Generated {len(tasks)} reconstruction tasks", level="info")
        return tasks

    def _validate_reconstruction_setup(self) -> None:
        """Validate reconstruction environment is properly setup."""
        # Check plugin directory exists
        plugin_dir = os.path.join(self.eicrecon_plugin_path, "EICrecon_MY")
        if not os.path.exists(plugin_dir):
            raise RuntimeError(f"EICrecon_MY plugin directory not found at: {plugin_dir}")
        
        # Check analyzeLumiHits.so exists
        plugin_lib = os.path.join(plugin_dir, "plugins/analyzeLumiHits.so") 
        if not os.path.exists(plugin_lib):
            raise RuntimeError(f"analyzeLumiHits plugin not found at: {plugin_lib}")

    def setup_readme(self) -> None:
        """
        Set up the README file with simulation information.
        """
        self.readme_path = os.path.join(self.backup_path, "README.txt")
        self.printlog(f"Setting up README file at: {self.readme_path}", level="info")

        try:
            # get BH value from settings
            bh_enabled = self.settings_dict.get('BH', 0)
            self.printlog(f"Retrieved BH value: {bh_enabled}", level="info")
            
            # get energy levels
            self.energy_vals = self.settings_dict.get('energy_levels', [])
            self.printlog(f"Extracted energy levels: {self.energy_vals}", level="info")
        
            # get DETECTOR_PATH value - use instance attribute if get_detector_path fails
            try:
                detector_path = self.get_detector_path()
            except Exception as e:
                self.printlog(f"Error getting detector path: {e}", level="warning")
                detector_path = getattr(self, 'detector_path', 'Not set')

            # check if executed from Singularity
            in_singularity, sif_path = self.check_singularity()
        
            # write the README content to the file
            self.printlog(f"Writing simulation information to README file: {self.readme_path}", level="info")
            with open(self.readme_path, 'w') as file:
                file.write("SIMULATION INFORMATION:\n")
                file.write(f"py_file: {os.path.basename(__file__)}\n")

                # Write settings
                for key, value in self.settings_dict.items():
                    file.write(f"{key}: {value}\n")
                    self.printlog(f"Added setting: {key}: {value}")

                file.write(f"DETECTOR_PATH: {detector_path}\n")
                self.printlog(f"Added DETECTOR_PATH: {detector_path}", level="info")

                file.write(f"Executed from Singularity: {in_singularity}\n")
                if (in_singularity):
                    file.write(f"SIF path: {sif_path}\n")
                    self.printlog(f"Added SIF path: {sif_path}", level="info")
                
                file.write("\n*********************\n")  
                self.printlog("Finished writing to README.", level="info")

        except Exception as e:
            self.printlog(f"An error occurred while setting up the README: {e}", level="error")
            raise

    def get_detector_path(self) -> str:
        """
        Get the detector path, either from environment variable or instance attribute.
        
        Returns:
            str: Path to the detector directory
        """
        # First try environment variable
        env_path = os.getenv('DETECTOR_PATH')
        if env_path and os.path.exists(env_path):
            self.printlog(f"Using DETECTOR_PATH from environment: {env_path}", level="info")
            return env_path
        
        # Fall back to instance attribute
        if hasattr(self, 'detector_path') and self.detector_path:
            self.printlog(f"Using detector_path from instance: {self.detector_path}", level="info")
            return self.detector_path
        
        self.printlog("No valid detector path found", level="warning")
        return "Not set"

    def check_singularity(self) -> Tuple[bool, str]:
        """
        Check if running inside Singularity container and get container path.
        
        Returns:
            Tuple[bool, str]: (is_in_singularity, singularity_path)
        """
        # Check if running in Singularity
        in_singularity = 'SINGULARITY_CONTAINER' in os.environ
        
        # Get container path if in Singularity
        sif_path = os.getenv('SINGULARITY_CONTAINER', '')
        
        # If not in Singularity but we have a path configured
        if not in_singularity and hasattr(self, 'singularity_image_path'):
            sif_path = self.singularity_image_path
        
        self.printlog(f"Singularity check - In container: {in_singularity}, Path: {sif_path}", level="debug")
        return in_singularity, sif_path

if __name__ == "__main__":

    """ Simulation """
    # initialize the simulation handler
    eic_simulation = HandleSim()
    eic_simulation.printlog("Simulation handler initialized.", level="info")

    # initialize paths, variables, and settings from JSON
    eic_simulation.printlog("Initializing settings, variables, and paths.", level="info")
    eic_simulation.init_vars()  
    eic_simulation.init_paths()  
    os.chmod(os.getcwd(), 0o777)
    eic_simulation.printlog("Settings, variables, and paths initialized.", level="info")

    # Check and create hepmc files first
    eic_simulation.printlog("Checking and creating hempc files.", level="info")
    eic_simulation.get_energies()
    eic_simulation.printlog("hempc file check completed.", level="info")

    # prepare the simulation based on settings (now working directly in backup location)
    eic_simulation.printlog("Preparing simulation based on settings.", level="info")
    eic_simulation.prep_sim()

    # execute the simulation and reconstruction in parallel
    eic_simulation.exec_sim() 

    # Only merge if reconstruction was successful
    """
    if eic_simulation.enable_reconstruction:
        eic_simulation.printlog("Merging reconstruction outputs.", level="info")
        eic_simulation.merge_recon_out()
    """

    # Create README directly
    eic_simulation.printlog("Creating README file.", level="info")
    eic_simulation.setup_readme()
    eic_simulation.printlog("Simulation process completed.", level="info")
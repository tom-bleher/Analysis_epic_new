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

class HandleSim(object):
    """
    Handle the main simulation farming
    """

    def __init__(self) -> None:
        # init internal variables
        self.energies: List[str] = []
        self.sim_dict: Dict[str, Dict[str, str]] = {}
        self.settings_path: str = "simulation_settings.json"
        self.execution_path: str = os.getcwd()
        
        # Create backup path early
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.backup_path: str = os.path.join(self.execution_path, "simEvents", timestamp)
        os.makedirs(self.backup_path, exist_ok=True)
        
        # init vars to be populated by the JSON file
        self.px_pairs: List[Tuple[float, float]] = []
        self.num_particles: int = 0
        self.eic_shell_path: str = ""
        self.det_path: str = ""
        self.file_types: str = ""
        self.hepmc_path: str = ""
        self.sim_out_path: str = ""
        self.det_ip6_path: str = ""
        self.reconstruct = False  
        self.console_logging = False 
        self.sif_path: str = ""  # Initialize sif_path
        self.plugin_path: str = ""  # Initialize plugin_path
        
        # Set up logger first without using printlog
        self.logger = None  # Initialize logger as None
        
        # Add validation for required paths
        self.required_paths = [
            'det_path',
            'hepmc_path',
            'sif_path'  # Add explicit check for sif_path
        ]
        
        # Add explicit validation for simulation settings
        self.required_simulation_settings = [
            'px_pairs',
            'num_particles',
            'det_path',
            'file_types',
            'hepmc_path',
            'sif_path'
        ]
        
        # Add validation for required paths when reconstruction is enabled
        self.reconstruct_required_paths = [
            'plugin_path'  # Add plugin path as required when reconstruction is enabled
        ]
        
        # Move overview.log to backup path after it's created
        self.overview_log_path: str = None  # Will be set after backup_path is created
        
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
        
        # Add file handler - now using backup_path
        self.overview_log_path = os.path.join(self.backup_path, "overview.log")
        file_handler = logging.FileHandler(self.overview_log_path, mode="w")
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(formatter)
        self.logger.addHandler(file_handler)
        
        # Add console handler if enabled, with plain formatting
        if self.console_logging:
            console_handler = logging.StreamHandler()
            console_handler.setLevel(logging.DEBUG)
            console_handler.setFormatter(formatter)
            self.logger.addHandler(console_handler)
            self.printlog("Console logging enabled.", level="info")

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

            # Validate px_pairs format
            if not isinstance(self.settings_dict['px_pairs'], list) or \
               not all(isinstance(pair, list) and len(pair) == 2 for pair in self.settings_dict['px_pairs']):
                raise ValueError("px_pairs must be a list of [dx, dy] pairs")

            # Load initial settings
            self.console_logging = self.settings_dict.get("console_logging", self.console_logging)
            print(f"console_logging set to: {self.console_logging}")  # Debug print statement

            # Load required settings
            required_keys = ["px_pairs", "num_particles", "det_path", "file_types", "hepmc_path", "reconstruct", "sif_path"]
            for key in required_keys:
                if key not in self.settings_dict or self.settings_dict[key] is None:
                    print(f"Missing or empty key: {key} in settings.")  # Debug print statement
                    raise ValueError(f"Missing or empty key: {key} in settings: {self.settings_dict.get(key)}")

            # set attributes dynamically
            for key, value in self.settings_dict.items():
                setattr(self, key, value)
                print(f"Set attribute {key} to {value}.")  # Debug print statement

            # Validate all required paths exist
            for path_key in self.required_paths:
                path_value = self.settings_dict.get(path_key)
                if not path_value or not os.path.exists(path_value):
                    raise ValueError(f"Required path '{path_key}' is missing or invalid: {path_value}")
            # Validate sif_path
            self.sif_path = self.settings_dict.get("sif_path")
            if not self.sif_path or not os.path.exists(self.sif_path):
                raise ValueError(f"Singularity image path 'sif_path' is missing or invalid: {self.sif_path}")

            # Add plugin_path to required settings if reconstruction is enabled
            if self.settings_dict.get("reconstruct", False):
                self.required_simulation_settings.append("plugin_path")

            # Additional validation for reconstruction-specific settings
            if self.settings_dict.get("reconstruct", False):
                # Validate plugin path exists if reconstruction is enabled
                plugin_path = self.settings_dict.get("plugin_path")
                if not plugin_path:
                    raise ValueError("plugin_path is required when reconstruct is enabled")
                
                # Check if plugin path exists and is accessible
                if not os.path.exists(plugin_path):
                    raise FileNotFoundError(f"Plugin path does not exist: {plugin_path}")
                if not os.path.isdir(plugin_path):
                    raise NotADirectoryError(f"Plugin path is not a directory: {plugin_path}")
                
                # Check if EICrecon_MY directory exists in plugin path
                eicrecon_my_path = os.path.join(plugin_path, "EICrecon_MY")
                if not os.path.exists(eicrecon_my_path):
                    raise FileNotFoundError(f"EICrecon_MY directory not found in plugin path: {eicrecon_my_path}")

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

    def init_paths(
        self
        ) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """
        self.printlog("Initializing paths.", level="info")
        self.settings_path = "simulation_settings.json"

        if not os.path.isdir(self.sim_out_path) or self.sim_out_path == "":
            self.sim_out_path = self.execution_path + "/simEvents"
        else:
            self.sim_out_path = self.settings_dict.get("sim_out_path")
        os.makedirs(self.sim_out_path, exist_ok=True)
        self.printlog(f"Simulation output path set to {self.sim_out_path}.", level="info")

        if not os.path.exists(self.hepmc_path):
            self.printlog(f"The specified hepmc path {self.hepmc_path} does not exist.", level="error")
            raise ValueError(f"The specified hepmc path {self.hepmc_path} does not exist. Run GenSimFiles.py and link its output correctly.")

        # create the path where the simulation file backup will go
        self.backup_path = os.path.join(self.sim_out_path , datetime.now().strftime("%Y%m%d_%H%M%S"))
        self.GenFiles_path = os.path.join(self.execution_path, "createGenFiles.py")
        if not os.path.isfile(self.GenFiles_path):
            self.printlog("Did not find your GenFiles.py file", level="error")
            raise

    def get_energies(self) -> None:
        """
        Check for required HepMC files and create them if missing.
        """
        self.printlog("Checking for required HepMC files...", level="info")
        
        # Create results directory if it doesn't exist
        os.makedirs(self.hepmc_path, exist_ok=True)
        
        # Check for missing files
        missing_files = []
        for energy in self.energies:
            for file_type in self.file_types:
                hepmc_file = f"{file_type}_{energy}.hepmc"
                if not os.path.exists(os.path.join(self.hepmc_path, hepmc_file)):
                    missing_files.append(hepmc_file)
        
        if missing_files:
            self.printlog(f"Missing HepMC files: {missing_files}", level="info")
            self.create_hepmc(missing_files)
        else:
            self.printlog("All required HepMC files found.", level="info")

    def create_hepmc(self, missing_files: List[str]) -> None:
        """
        Create missing HepMC files following the same process as createGenFiles2.py
        """
        self.printlog("Creating missing HepMC files...", level="info")
        
        # Get paths to required macros and utilities
        macro_dir = os.path.dirname(__file__)
        utilities_dir = os.path.join(os.path.dirname(macro_dir), "utilities")
        lumi_macro = os.path.join(macro_dir, "lumi_particles.cxx")
        prop_macro = os.path.join(macro_dir, "PropagateAndConvert.cxx")
        
        # Verify required files and directories exist
        if not os.path.exists(lumi_macro):
            raise FileNotFoundError(f"Required macro not found: {lumi_macro}")
        if not os.path.exists(prop_macro):
            raise FileNotFoundError(f"Required macro not found: {prop_macro}")
        if not os.path.exists(utilities_dir):
            raise FileNotFoundError(f"Required utilities directory not found: {utilities_dir}")
        if not os.path.exists(os.path.join(utilities_dir, "constants.h")):
            raise FileNotFoundError(f"Required constants.h not found in {utilities_dir}")
        
        for file_info in missing_files:
            try:
                file_type, energy = file_info.replace('.hepmc', '').split('_')
                energy = int(energy)
                
                self.printlog(f"\nProcessing {file_type} at {energy} GeV", level="info")
                self.printlog("Following createGenFiles2.py workflow:", level="info")
                
                # First create idealPhotonsAtIP file if needed (common step for both types)
                ideal_photons_file = os.path.join(self.hepmc_path, f"idealPhotonsAtIP_{energy}.hepmc")
                if not os.path.exists(ideal_photons_file):
                    self.printlog("Step 1: Creating ideal photons (required intermediate step)", level="info")
                    # Generate ideal photons using lumi_particles.cxx
                    cmd = f"root -b -q '{lumi_macro}(1e4,true,false,false,{energy},{energy},\"{ideal_photons_file}\")'"
                    self.printlog(f"Generating ideal photons with command: {cmd}", level="debug")
                    
                    # Run in singularity with proper bindings
                    singularity_cmd = [
                        "singularity", "exec", "--containall",
                        "--bind", f"{os.path.dirname(self.hepmc_path)}:{os.path.dirname(self.hepmc_path)}",
                        "--bind", f"{macro_dir}:{macro_dir}",
                        "--bind", f"{utilities_dir}:{utilities_dir}",  # Add utilities binding
                        self.sif_path,
                        "bash", "-c", 
                        f"cd {macro_dir} && {cmd}"  # Change to macro directory before running
                    ]
                    
                    result = subprocess.run(singularity_cmd, capture_output=True, text=True)
                    if result.returncode != 0:
                        self.printlog(f"Command failed with output:\n{result.stdout}\n{result.stderr}", level="error")
                        raise RuntimeError(f"Failed to create {ideal_photons_file}")
                    
                    if not os.path.exists(ideal_photons_file):
                        raise RuntimeError(f"Failed to create {ideal_photons_file}")
                else:
                    self.printlog("Step 1: Using existing ideal photons file", level="info")
                
                if file_type == 'beamEffectsElectrons':
                    self.printlog("Step 2a: Adding beam effects to photons (intermediate step)", level="info")
                    # Create beam effects photons first
                    beam_effects_photons = os.path.join(self.hepmc_path, f"beamEffectsPhotonsAtIP_{energy}.hepmc")
                    cmd = f"abconv {ideal_photons_file} --plot-off -o {os.path.splitext(beam_effects_photons)[0]}"
                    self.printlog(f"Generating beam effects photons with command: {cmd}", level="debug")
                    
                    singularity_cmd = [
                        "singularity", "exec", "--containall",
                        "--bind", f"{os.path.dirname(self.hepmc_path)}:{os.path.dirname(self.hepmc_path)}",
                        self.sif_path,
                        "bash", "-c", cmd
                    ]
                    
                    result = subprocess.run(singularity_cmd, capture_output=True, text=True)
                    if result.returncode != 0:
                        self.printlog(f"Command failed with output:\n{result.stdout}\n{result.stderr}", level="error")
                        raise RuntimeError(f"Failed to create {beam_effects_photons}")
                    
                    self.printlog("Step 2b: Converting beam-affected photons to electrons (final step)", level="info")
                    # Then propagate to electrons
                    output_file = os.path.join(self.hepmc_path, f"beamEffectsElectrons_{energy}.hepmc")
                    cmd = f"root -b -q '{prop_macro}(\"{beam_effects_photons}\",\"{output_file}\",{self.location})'"
                    self.printlog(f"Propagating to electrons with command: {cmd}", level="debug")
                    
                    singularity_cmd = [
                        "singularity", "exec", "--containall",
                        "--bind", f"{os.path.dirname(self.hepmc_path)}:{os.path.dirname(self.hepmc_path)}",
                        "--bind", f"{macro_dir}:{macro_dir}",
                        "--bind", f"{utilities_dir}:{utilities_dir}",  # Add utilities binding
                        self.sif_path,
                        "bash", "-c",
                        f"cd {macro_dir} && {cmd}"  # Change to macro directory before running
                    ]
                    
                    result = subprocess.run(singularity_cmd, capture_output=True, text=True)
                    if result.returncode != 0:
                        self.printlog(f"Command failed with output:\n{result.stdout}\n{result.stderr}", level="error")
                        raise RuntimeError(f"Failed to create {output_file}")
                
                elif file_type == 'idealElectrons':
                    self.printlog("Step 2: Converting ideal photons to electrons (final step)", level="info")
                    # Propagate ideal photons to electrons
                    output_file = os.path.join(self.hepmc_path, f"idealElectrons_{energy}.hepmc")
                    cmd = f"root -b -q '{prop_macro}(\"{ideal_photons_file}\",\"{output_file}\",{self.location})'"
                    self.printlog(f"Propagating to electrons with command: {cmd}", level="debug")
                    
                    singularity_cmd = [
                        "singularity", "exec", "--containall",
                        "--bind", f"{os.path.dirname(self.hepmc_path)}:{os.path.dirname(self.hepmc_path)}",
                        "--bind", f"{macro_dir}:{macro_dir}",
                        "--bind", f"{utilities_dir}:{utilities_dir}",  # Add utilities binding
                        self.sif_path,
                        "bash", "-c",
                        f"cd {macro_dir} && {cmd}"  # Change to macro directory before running
                    ]
                    
                    result = subprocess.run(singularity_cmd, capture_output=True, text=True)
                    if result.returncode != 0:
                        self.printlog(f"Command failed with output:\n{result.stdout}\n{result.stderr}", level="error")
                        raise RuntimeError(f"Failed to create {output_file}")
                
                # Verify file creation
                final_file = os.path.join(self.hepmc_path, file_info)
                if not os.path.exists(final_file):
                    raise RuntimeError(f"Failed to create {final_file}")
                    
                self.printlog(f"Completed processing {file_type} at {energy} GeV", level="info")
                
            except Exception as e:
                error_msg = f"Error generating {file_info}: {str(e)}"
                self.printlog(error_msg, level="error")
                raise RuntimeError(error_msg)

    def _run_in_singularity(self, cmd: str, file_desc: str) -> None:
        """Run a command in Singularity with proper bindings."""
        self.printlog(f"Running command for {file_desc}", level="debug")
        
        # Prepare Singularity command
        singularity_cmd = [
            "singularity", "exec", "--containall",
            "--bind", f"{os.path.dirname(self.hepmc_path)}:{os.path.dirname(self.hepmc_path)}",
            "--bind", f"{os.path.dirname(__file__)}:{os.path.dirname(__file__)}",
            "--bind", f"{os.path.dirname(os.path.dirname(__file__))}:{os.path.dirname(os.path.dirname(__file__))}",
            self.sif_path,
            "/bin/bash", "-c", cmd
        ]
        
        try:
            # Execute command
            process = subprocess.Popen(
                singularity_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            
            # Monitor output in real-time
            for line in iter(process.stdout.readline, ''):
                line = line.strip()
                if line:
                    self.printlog(f"Command output: {line}", level="debug")
            
            for line in iter(process.stderr.readline, ''):
                line = line.strip()
                if line:
                    self.printlog(f"Command stderr: {line}", level="debug")
            
            # Wait for process to complete
            process.wait()
            if process.returncode != 0:
                raise subprocess.CalledProcessError(process.returncode, cmd)
            
            self.printlog(f"Successfully completed command for {file_desc}", level="info")
            
        except Exception as e:
            self.printlog(f"Command failed for {file_desc}: {str(e)}", level="error")
            raise
        finally:
            # Ensure process is cleaned up
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
                self.printlog("Cleaned up subprocess", level="debug")

    def _execute_commands_for_energy(self, energy: str, commands: List[Tuple[str, str]]) -> None:
        """Execute commands for a specific energy level in the correct order."""
        self.printlog(f"Processing commands for energy {energy}", level="info")
        
        # Sort commands by type to ensure correct execution order
        order = {"photons": 0, "beam_effects": 1, "propagate": 2}
        sorted_commands = sorted(commands, key=lambda x: order[x[0]])
        
        for cmd_type, cmd in sorted_commands:
            try:
                self.printlog(f"Executing {cmd_type} command for energy {energy}: {cmd}", level="debug")
                result = subprocess.run(cmd, shell=True, check=True, capture_output=True, text=True)
                
                if result.stdout:
                    self.printlog(f"Command output: {result.stdout}", level="debug")
                if result.stderr:
                    self.printlog(f"Command stderr: {result.stderr}", level="debug")
                    
            except subprocess.CalledProcessError as e:
                self.printlog(f"Command failed: {e.stderr}", level="error")
                raise RuntimeError(f"Failed to execute {cmd_type} command for energy {energy}")

    def init_vars(
        self
        ) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """

        # default if user does not provide JSON
        self.def_set_dict = {
            "px_pairs": [[1.0,0.1],[0.1,0.1],[2.0,0.1]],
            "energies": [10,15,20,25,30],
            "num_particles": 100000,
            "eic_shell_path": "/data/tomble/eic",
            "det_path": "/data/tomble/eic/epic",
            "file_types": "idealElectrons",
            "hepmc_path": "/data/tomble/Analysis_epic_new/simulations/genEvents/results",
            "sif_path": "/data/tomble/eic/local/lib/eic_xl-nightly.sif",
            "plugin_path": "/data/tomble/Analysis_epic_new/EICreconPlugins",
            "reconstruct": True,
            "console_logging": True,
            "sim_out_path": "",
            "det_ip6_path": "",
            "sim_description": ""
        }

        # check that all boolean settings are correct
        bool_keys = ["reconstruct", "console_logging"]
        for bkey in bool_keys:
            if not hasattr(self, bkey):
                self.printlog(f"Missing key: '{bkey}' in settings.", level="error")
                raise KeyError(f"Missing key: '{bkey}' in settings.")
            
            # Get the value of the attribute and convert to boolean if it's a string
            value = getattr(self, bkey)

            # If the value is a string that represents a boolean, convert it
            if isinstance(value, str):
                if value.lower() == "true":
                    value = True
                elif value.lower() == "false":
                    value = False
                else:
                    self.printlog(f"Invalid value for '{bkey}' in settings. Expected 'True' or 'False' as a string.", level="error")
                    raise ValueError(f"Invalid value for '{bkey}' in settings. Expected 'True' or 'False' as a string.")
            
            # Now check if the value is actually a boolean
            if not isinstance(value, bool):
                self.printlog(f"Invalid value for '{bkey}' in settings. Expected a boolean (True or False).", level="error")
                raise ValueError(f"Invalid value for '{bkey}' in settings. Expected a boolean (True or False).")
            
            # Set the attribute to the correct boolean value
            setattr(self, bkey, value)
            self.printlog(f"Set boolean attribute {bkey} to {value}.", level="debug")

    def prep_sim(
        self
        ) -> None:
        """
        Prepare simulation environment with enhanced error handling.
        """
        self.printlog("Preparing simulation.", level="info")
        
        # Validate px_pairs before starting
        if not self.px_pairs:
            raise ValueError("No pixel pairs defined for simulation")
            
        # Create simulation root directory if it doesn't exist
        os.makedirs(self.backup_path, exist_ok=True)
        
        # loop over all requested detector changes and create specifics
        self.printlog(f"Starting simulation loop for px_pairs for init_specifics: {self.px_pairs}", level="info")
        for curr_px_dx, curr_px_dy in self.px_pairs:
            
            """ soft changes """
            # create respective px folder in backup path instead of sim_out_path
            curr_sim_path = os.path.join(self.backup_path, f"{curr_px_dx}x{curr_px_dy}px")
            try:
                os.makedirs(curr_sim_path, exist_ok=True)
                self.printlog(f"Created simulation path: {curr_sim_path}", level="info")

                # copy epic folder to current pixel path
                curr_sim_det_path = self.copy_epic(curr_sim_path)

                """ hard changes """
                # rewrite detector's XMLs to hold current change for detector
                self.mod_detector_settings(curr_sim_det_path, curr_px_dx, curr_px_dy)
                
                """ gather simulation relavent details """
                # update the simulation dictionary for current requested change
                single_sim_dict = self.create_sim_dict(curr_sim_path, curr_sim_det_path, curr_px_dx, curr_px_dy)
                self.sim_dict.update(single_sim_dict)
                self.printlog(f"Updated simulation dictionary with key {curr_px_dx}x{curr_px_dy}.", level="info")
                self.printlog(f"Current simulation dictionary: {json.dumps(self.sim_dict, indent=2)}", level="debug")
            except Exception as e:
                self.printlog(f"Failed to prepare simulation for {curr_px_dx}x{curr_px_dy}: {e}", level="error")
                raise

    def create_sim_dict(self, curr_sim_path: str, curr_sim_det_path: str, curr_px_dx: float, curr_px_dy: float) -> Dict[str, dict[str, str]]:
        """
        Create simulation dictionary holding relevant parameters with improved command structure
        """
        self.printlog(f"Creating simulation dictionary for {curr_px_dx}x{curr_px_dy}.", level="info")
        
        # Initialize dict to hold parameters for one simulation
        single_sim_dict = {}
        px_key = f"{curr_px_dx}x{curr_px_dy}"

        # Create base dictionary entry with paths
        single_sim_dict[px_key] = {
            "sim_det_path": curr_sim_det_path,
            "sim_compact_path": os.path.join(curr_sim_det_path, "install/share/epic/compact"),
            "sim_ip6_path": os.path.join(curr_sim_det_path, "install/share/epic/epic_ip6_extended.xml"),
            "sim_shell_path": os.path.join(curr_sim_det_path, "install/bin/thisepic.sh"),
            "ddsim_cmds": [],
            "recon_cmds": [] if self.reconstruct else None,
            "task_ids": []  # Add task IDs for tracking
        }

        # Generate commands for each combination
        for file_type in self.file_types:
            for energy in self.energies:
                # Create unique task ID
                task_id = f"{curr_px_dx}x{curr_px_dy}_{file_type}_{energy}"
                single_sim_dict[px_key]["task_ids"].append(task_id)
                
                # Verify input file exists
                input_file = os.path.join(self.hepmc_path, f"{file_type}_{energy}.hepmc")
                if not os.path.exists(input_file):
                    self.printlog(f"Warning: Input file not found: {input_file}", level="warning")
                    continue

                # Setup output paths
                sim_output = os.path.join(curr_sim_path, f"output_{file_type}_{energy}edm4hep.root")
                os.makedirs(os.path.dirname(sim_output), exist_ok=True)

                # Generate ddsim command
                ddsim_cmd = self.get_ddsim_cmd(input_file, sim_output, single_sim_dict[px_key]["sim_ip6_path"])
                single_sim_dict[px_key]["ddsim_cmds"].append(ddsim_cmd)
                self.printlog(f"Added ddsim command for {task_id}", level="debug")

                # Generate reconstruction command if enabled
                if self.reconstruct:
                    recon_dir = os.path.join(curr_sim_path, "recon")
                    os.makedirs(recon_dir, exist_ok=True)
                    recon_output = os.path.join(recon_dir, f"recon_output_{file_type}_{energy}edm4hep.root")
                    recon_cmd = self.get_recon_cmd(sim_output, recon_output, curr_sim_det_path)
                    single_sim_dict[px_key]["recon_cmds"].append(recon_cmd)
                    self.printlog(f"Added reconstruction command for {task_id}", level="debug")

        # Log command generation summary
        self.printlog(
            f"Generated {len(single_sim_dict[px_key]['ddsim_cmds'])} ddsim commands and "
            f"{len(single_sim_dict[px_key]['recon_cmds'] or [])} reconstruction commands for {px_key}",
            level="info"
        )

        return single_sim_dict

    def get_ddsim_cmd(self, input_file: str, output_file: str, compact_file: str) -> str:
        """
        Generate a ddsim command with consistent parameters
        """
        return (
            f"ddsim --inputFiles {input_file} "
            f"--outputFile {output_file} "
            f"--compactFile {compact_file} "
            f"-N {self.num_particles} "
            #f"--runType batch "
            #f"--printLevel DEBUG "
            #f"--enableDetailedShowerMode "
            #f"--physics.list FTFP_BERT"
        )

    def get_recon_cmd(self, input_file: str, output_file: str, detector_path: str) -> str:
        """
        Generate a reconstruction command with consistent parameters
        """
        return (
            f"eicrecon "
            f"-Pplugins=analyzeLumiHits "
            #f"-Pdd4hep_dir={detector_path}/install "
            #f"-Ppodio:output_collections=ReconstructedParticles,EcalEndcapNRawHits,EcalBarrelHits "
            f"-Phistsfile={output_file} {input_file}"
            #f"-o {output_file}"
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
                self.sif_path,
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
        Copy and compile detector to simulation directory.
        """
        self.printlog(f"Copying epic detector to {curr_sim_path}.", level="info")
        try:
            det_name = os.path.basename(self.det_path)
            dest_path = os.path.join(curr_sim_path, det_name)
            
            # Copy fresh detector
            if os.path.exists(dest_path):
                shutil.rmtree(dest_path)
            shutil.copytree(self.det_path, dest_path, symlinks=True)
            
            # Compile the copied detector
            self.compile_epic(dest_path)
            
            self.printlog(f"Successfully copied and compiled detector at {dest_path}", level="info")
            return dest_path
                
        except Exception as e:
            self.printlog(f"Failed to copy/compile detector: {e}", level="error")
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

    def exec_sim(self) -> None:
        """
        Execute simulation and reconstruction commands with improved task tracking
        """
        task_status = {}
        failed_tasks = []
        
        # Process each pixel pair
        for px_key, sim_data in self.sim_dict.items():
            self.printlog(f"Processing pixel pair: {px_key}", level="info")
            
            # Create directories
            px_path = os.path.join(self.backup_path, f"{px_key}px")
            os.makedirs(px_path, exist_ok=True)
            
            # Process ddsim commands first
            for i, ddsim_cmd in enumerate(sim_data["ddsim_cmds"]):
                task_id = sim_data["task_ids"][i]
                try:
                    self.printlog(f"Executing ddsim for {task_id}", level="info")
                    result = self.run_single_sim(
                        ddsim_cmd,
                        sim_data["sim_det_path"],
                        sim_data["sim_shell_path"],
                        px_key
                    )
                    task_status[task_id] = {"status": "completed", "message": result}
                except Exception as e:
                    error_msg = f"Failed ddsim for {task_id}: {str(e)}"
                    self.printlog(error_msg, level="error")
                    task_status[task_id] = {"status": "failed", "error": str(e)}
                    failed_tasks.append(task_id)
                    continue
                
                # Run reconstruction if enabled and ddsim succeeded
                if self.reconstruct and task_status[task_id]["status"] == "completed":
                    recon_task_id = f"recon_{task_id}"
                    try:
                        recon_cmd = sim_data["recon_cmds"][i]
                        self.printlog(f"Executing reconstruction for {recon_task_id}", level="info")
                        result = self.run_reconstruction(recon_cmd, px_key)
                        task_status[recon_task_id] = {"status": "completed", "message": result}
                    except Exception as e:
                        error_msg = f"Failed reconstruction for {recon_task_id}: {str(e)}"
                        self.printlog(error_msg, level="error")
                        task_status[recon_task_id] = {"status": "failed", "error": str(e)}
                        failed_tasks.append(recon_task_id)

        # Generate execution report and log summary
        self.create_execution_report(task_status)
        self.log_execution_summary(task_status, failed_tasks)

    def get_recon_cmd_for_energy(self, px_key: str, energy: str) -> dict:
        """
        Generate reconstruction command for specific pixel key and energy level.
        """
        # Create a list to store commands for each file type
        commands = []
        for file_type in self.file_types:  # Use self.file_types instead of self.file_type
            sim_file = os.path.join(
                self.backup_path,
                f"{px_key}px",
                f"output_{file_type}_{energy}edm4hep.root"
            )
            
            # Get the detector path from sim_dict
            detector_path = self.sim_dict[px_key]['sim_det_path']
            
            if os.path.exists(sim_file):
                cmd = self.get_recon_cmd(
                    curr_sim_path=os.path.dirname(sim_file),
                    sim_file=sim_file,
                    detector_path=detector_path
                )
                if cmd:
                    commands.append(cmd)
        
        return commands if commands else None

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
            f.write(f"Number of particles: {self.num_particles}\n")
            f.write(f"File types: {', '.join(self.file_types)}\n")
            f.write(f"Energies: {', '.join(map(str, self.energies))}\n")
            f.write(f"Reconstruction enabled: {self.reconstruct}\n\n")
            
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
                if self.reconstruct:
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

    def log_execution_summary(self, task_status: dict, failed_tasks: list) -> None:
        """
        Log summary of execution results.
        """
        total = len(task_status)
        completed = sum(1 for status in task_status.values() if status['status'] == 'completed')
        failed = len(failed_tasks)
        
        self.printlog(f"""
        Execution Summary:
        -----------------
        Total Tasks: {total}
        Completed: {completed}
        Failed: {failed}
        Success Rate: {(completed/total)*100:.2f}%
        """, level="info")

    def setup_subprocess_logger(self, sim_cmd: str, px_key: str) -> Tuple[logging.Logger, str]:
        """
        Create a separate logger for each subprocess with detailed identification.
        """
        # Extract file type and energy from command for better logging identification
        file_type = None
        energy = None
        
        # Initialize log_file with a default value
        log_file = None
        
        # Parse command to get file type and energy
        if '--inputFiles' in sim_cmd:
            # Handle ddsim command
            input_file = sim_cmd.split('--inputFiles')[1].split()[0]
            base_name = os.path.basename(input_file)
            if base_name.endswith('.hepmc'):
                parts = base_name.replace('.hepmc', '').split('_')
                if len(parts) >= 2:
                    file_type = parts[0]
                    energy = parts[1]
        elif 'recon_output_' in sim_cmd:
            # Handle reconstruction command
            output_parts = [p for p in sim_cmd.split() if 'recon_output_' in p][0]
            parts = os.path.basename(output_parts).split('_')
            if len(parts) >= 3:
                file_type = parts[1]
                energy = parts[2].replace('edm4hep.root', '')
        
        # Create unique logger name
        process_id = f"{px_key}_{file_type}_{energy}_pid{os.getpid()}" if file_type and energy else f"{px_key}_pid{os.getpid()}"
        logger_name = f"subprocess_{process_id}"
        logger = logging.getLogger(logger_name)
        
        if not logger.hasHandlers():
            logger.setLevel(logging.DEBUG)
            
            # Create log file in the pixel-specific directory within backup_path
            px_path = os.path.join(self.backup_path, f"{px_key}px")
            os.makedirs(px_path, exist_ok=True)
            
            # Create logs directory for better organization
            logs_dir = os.path.join(px_path, "logs")
            os.makedirs(logs_dir, exist_ok=True)
            
            # Create specific log file for this process
            if file_type and energy:
                log_file = os.path.join(logs_dir, f"{file_type}_{energy}_subprocess.log")
            else:
                log_file = os.path.join(logs_dir, f"subprocess_{os.getpid()}.log")
        
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
        
        # Ensure log_file is always returned, even if handlers already exist
        if not log_file:
            log_file = os.path.join(self.backup_path, f"{px_key}px", "logs", f"subprocess_{os.getpid()}.log")
        
        return logger, log_file

    def run_single_sim(self, sim_cmd: str, detector_path: str, shell_path: str, px_key: str) -> str:
        """
        Execute a single simulation with enhanced logging.
        """
        subprocess_logger, log_file = self.setup_subprocess_logger(sim_cmd, px_key)
        
        try:
            # Extract file type and energy from command for better error reporting
            file_type = "unknown"
            energy = "unknown"
            if '--inputFiles' in sim_cmd:
                input_file = sim_cmd.split('--inputFiles')[1].split()[0]
                base_name = os.path.basename(input_file)
                if base_name.endswith('.hepmc'):
                    parts = base_name.replace('.hepmc', '').split('_')
                    if len(parts) >= 2:
                        file_type = parts[0]
                        energy = parts[1]
            
            subprocess_logger.info(f"Starting simulation for {file_type} at {energy} GeV")
            
            # Validate paths
            if not os.path.exists(detector_path):
                raise FileNotFoundError(f"Detector path not found: {detector_path}")
            if not os.path.exists(self.sif_path):
                raise FileNotFoundError(f"Singularity image not found: {self.sif_path}")
            
            # Get absolute paths
            abs_detector_path = os.path.abspath(detector_path)
            abs_detector_parent = os.path.dirname(abs_detector_path)
            
            # Extract output file path from sim_cmd
            output_file = None
            for part in sim_cmd.split():
                if part.endswith('edm4hep.root'):
                    output_file = part
                    break
            
            if output_file:
                # Ensure output directory exists
                output_dir = os.path.dirname(output_file)
                os.makedirs(output_dir, exist_ok=True)
                subprocess_logger.info(f"Created output directory: {output_dir}")
            
            # Create command to execute run_sim.sh with required parameters
            script_path = os.path.join(os.path.dirname(__file__), "run_sim.sh")
            cmd = [
                script_path,
                abs_detector_path,
                abs_detector_parent,
                self.execution_path,
                self.sim_out_path,
                self.sif_path,
                sim_cmd,
                str(self.reconstruct).lower(),
                self.plugin_path if self.reconstruct else ""
            ]
            
            # Execute the script with enhanced error capture
            subprocess_logger.info(f"Starting simulation with command: {' '.join(cmd)}")
            
            # Create temporary files for output capture
            with tempfile.NamedTemporaryFile(mode='w+', suffix='.out') as stdout_file, \
                 tempfile.NamedTemporaryFile(mode='w+', suffix='.err') as stderr_file:
                
                process = subprocess.Popen(
                    cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    bufsize=1
                )
                
                # Define patterns for actual errors vs build output
                error_patterns = [
                    "Error:", "ERROR:", "Fatal:", "FATAL:",
                    "failed", "Failed", "FAILED",
                    "undefined reference", "Segmentation fault",
                    "Exception", "cannot find", "not found"
                ]
                
                # Collect all output for error analysis
                stdout_lines = []
                stderr_lines = []
                error_found = False
                
                # Monitor output in real-time
                for line in iter(process.stdout.readline, ''):
                    line = line.strip()
                    stdout_lines.append(line)
                    stdout_file.write(line + '\n')
                    subprocess_logger.info(line)
                    if any(pattern in line for pattern in error_patterns):
                        error_found = True
                
                for line in iter(process.stderr.readline, ''):
                    line = line.strip()
                    stderr_lines.append(line)
                    stderr_file.write(line + '\n')
                    if any(pattern in line for pattern in error_patterns):
                        subprocess_logger.error(line)
                        error_found = True
                    else:
                        subprocess_logger.info(line)
                
                process.wait()
                
                # Enhanced error reporting
                if process.returncode != 0 or error_found:
                    error_msg = "Simulation failed with the following errors:\n"
                    
                    # Rewind files for reading
                    stdout_file.seek(0)
                    stderr_file.seek(0)
                    
                    # Add stderr content
                    stderr_content = stderr_file.read()
                    if stderr_content:
                        error_msg += "\nStandard Error Output:\n" + stderr_content
                    
                    # Add relevant stdout content
                    stdout_content = stdout_file.read()
                    if stdout_content:
                        error_msg += "\nStandard Output (last 50 lines):\n" + \
                                   "\n".join(stdout_content.splitlines()[-50:])
                    
                    raise RuntimeError(error_msg)
            
            # Verify output file was created with enhanced checking
            if output_file:
                if not os.path.exists(output_file):
                    # Check directory contents for debugging
                    dir_contents = os.listdir(os.path.dirname(output_file))
                    raise RuntimeError(
                        f"Simulation completed but output file not found: {output_file}\n"
                        f"Directory contents: {dir_contents}\n"
                        f"Last command output:\n" + "\n".join(stdout_lines[-20:])
                    )
                
                # Check file size
                file_size = os.path.getsize(output_file)
                if file_size < 1000:  
                    raise RuntimeError(f"Output file too small ({file_size} bytes): {output_file}")
                
                subprocess_logger.info(f"Successfully created output file: {output_file} ({file_size} bytes)")
            
            subprocess_logger.info(f"Simulation completed successfully for {file_type} at {energy} GeV")
            return f"Successfully completed simulation for {px_key} ({file_type} at {energy} GeV)"
            
        except Exception as e:
            error_msg = f"Error in simulation for {px_key} ({file_type} at {energy} GeV): {str(e)}"
            subprocess_logger.error(error_msg)
            raise RuntimeError(error_msg) from e

    def run_reconstruction(self, recon_cmd: str, px_key: str) -> str:
        """
        Execute reconstruction with enhanced logging.
        """
        subprocess_logger, log_file = self.setup_subprocess_logger(recon_cmd, px_key)
        
        # Extract file type and energy from reconstruction command
        file_type = "unknown"
        energy = "unknown"
        if 'output_' in recon_cmd:
            output_file = [part for part in recon_cmd.split() if 'output_' in part][0]
            parts = os.path.basename(output_file).split('_')
            if len(parts) >= 3:
                file_type = parts[1]
                energy = parts[2].replace('edm4hep.root', '')
        
        subprocess_logger.info(f"Starting reconstruction for {file_type} at {energy} GeV")
        
        max_retries = 3
        retry_count = 0
        
        while retry_count < max_retries:
            try:
                self.run_reconstruction(recon_cmd, px_key) # TEST
                subprocess_logger.info(f"Reconstruction completed successfully for {file_type} at {energy} GeV")
                return f"Reconstruction completed successfully for {px_key} ({file_type} at {energy} GeV)"
                
            except Exception as e:
                retry_count += 1
                subprocess_logger.error(
                    f"Error in reconstruction (attempt {retry_count}/{max_retries}) "
                    f"for {file_type} at {energy} GeV: {str(e)}"
                )
                if retry_count < max_retries:
                    subprocess_logger.info("Retrying in 5 seconds...")
                    time.sleep(5)
                else:
                    subprocess_logger.error("Maximum retries reached, failing")
                    raise

    def verify_reconstruction_output(self, output_file: str) -> bool:
        """
        Verify that reconstruction output exists and is valid
        """
        if not os.path.exists(output_file):
            self.printlog(f"Reconstruction output file not found: {output_file}", level="error")
            return False
            
        # Check file size
        file_size = os.path.getsize(output_file)
        if file_size < 1000:
            self.printlog(f"Reconstruction output file too small: {output_file} ({file_size} bytes)", level="error")
            return False
        
        return True

    def get_detector_path(self) -> str:
        """
        Retrieve the value of the DETECTOR_PATH environment variable.
        """
        self.printlog("Retrieving DETECTOR_PATH environment variable.", level="info")
        detector_path = os.getenv('DETECTOR_PATH', 'Not set')
        self.printlog(f"DETECTOR_PATH: {detector_path}", level="info")
        return detector_path

    def check_singularity(self) -> Tuple[bool, str]:
        """
        Check if the script is executed from Singularity and return the .sif path if applicable.
        """
        self.printlog("Checking if executed from Singularity.", level="info")
        sif_path = os.getenv('SINGULARITY_CONTAINER', '')
        in_singularity = bool(sif_path)
        self.printlog(f"Executed from Singularity: {in_singularity}, SIF path: {sif_path}", level="info")
        return in_singularity, sif_path

    def setup_readme(
        self
        ) -> None:
        """
        Set up the README file with simulation information.
        """
        # define path for readme file 
        self.readme_path = os.path.join(self.backup_path , "README.txt")
        self.printlog(f"Setting up README file at: {self.readme_path}", level="info")

        try:
            # get BH value
            if self.BH:
                self.printlog(f"Retrieved BH value: {self.BH_val}", level="info")
            
            # get energy levels
            self.energy_vals = self.settings_dict.get('energies', [])
            self.printlog(f"Extracted energy levels: {self.energy_vals}", level="info")
        
            # get DETECTOR_PATH value
            detector_path = self.get_detector_path()

            # check if executed from Singularity
            in_singularity, sif_path = self.check_singularity()
        
            # write the README content to the file
            self.printlog(f"Writing simulation information to README file: {self.readme_path}", level="info")
            with open(self.readme_path, 'a') as file:
                file.write("SIMULATION INFORMATION:\n")

                file.write(f"py_file: {os.path.basename(__file__)}\n")
                self.printlog(f"Added py_file: {self.file_types}", level="info")

                # write each key-value pair from the settings_dict
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

    def success_recon(self, recon_path: str) -> bool:
        """
        Enhanced reconstruction output validation
        """
        self.printlog(f"Checking reconstruction output at {recon_path}", level="info")
        
        if not os.path.exists(recon_path):
            self.printlog(f"Reconstruction file does not exist: {recon_path}", level="error")
            return False
            
        try:
            # Check both file size and ROOT file validity
            result = subprocess.run(
                ["root", "-l", "-b", "-q", f"'{recon_path}?'"], 
                capture_output=True,
                text=True,
                check=False
            )
            
            # Check if file can be opened by ROOT
            if "Error" in result.stderr or "Error" in result.stdout:
                self.printlog(f"ROOT cannot open file: {recon_path}", level="error")
                return False
                
            # Check file size
            filesize = os.path.getsize(recon_path)
            if filesize < 1000:
                self.printlog(f"File too small ({filesize} bytes): {recon_path}", level="error")
                return False
                
            self.printlog(f"Valid reconstruction file found: {recon_path}", level="info")
            return True
            
        except Exception as e:
            self.printlog(f"Error validating reconstruction file: {e}", level="error")
            return False

    def merge_recon_out(self):
        """
        Modified reconstruction output merging with enhanced path debugging
        """
        self.printlog("Merging reconstruction output files.", level="info")
        
        for px_key, sim_data in self.sim_dict.items():
            px_folder = os.path.join(self.backup_path, f"{px_key}px")
            recon_dir = os.path.join(px_folder, "recon")
            
            # Enhanced path debugging
            self.printlog(f"Checking paths for {px_key}:", level="debug")
            self.printlog(f"Pixel folder: {px_folder}", level="debug")
            self.printlog(f"Reconstruction directory: {recon_dir}", level="debug")
            
            if not os.path.exists(recon_dir):
                self.printlog(f"Reconstruction directory not found: {recon_dir}", level="warning")
                continue
            
            # Find all reconstruction outputs for each file type
            recon_files = []
            for file_type in self.file_types:  
                for root, _, files in os.walk(recon_dir):
                    self.printlog(f"Searching in: {root}", level="debug")
                    for file in files:
                        if file.startswith(f"recon_{file_type}_") and file.endswith(".root"):
                            full_path = os.path.join(root, file)
                            if self.success_recon(full_path):
                                recon_files.append(full_path)
                                self.printlog(f"Found valid reconstruction file: {full_path}", level="debug")
                            else:
                                self.printlog(f"Invalid or incomplete reconstruction file: {full_path}", level="warning")
            
            if not recon_files:
                self.printlog(f"No valid reconstruction files found for {px_key}", level="warning")
                continue
            
            # Create merged output
            merged_output = os.path.join(recon_dir, f"merged_{px_key}.root")
            self.printlog(f"Merging {len(recon_files)} files into {merged_output}", level="info")
            
            merge_cmd = ["hadd", "-f", merged_output] + recon_files
            
            try:
                self.printlog(f"Executing merge command: {' '.join(merge_cmd)}", level="debug")
                result = subprocess.run(merge_cmd, check=True, capture_output=True, text=True)
                
                if result.stdout:
                    self.printlog(f"Merge output: {result.stdout}", level="debug")
                    
                self.printlog(f"Successfully merged reconstruction files for {px_key}", level="info")
                
            except subprocess.CalledProcessError as e:
                self.printlog(f"Failed to merge reconstruction files for {px_key}: {e.stderr}", level="error")
                raise

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
    if eic_simulation.reconstruct:
        eic_simulation.printlog("Merging reconstruction outputs.", level="info")
        eic_simulation.merge_recon_out()
    """

    # Create README directly
    eic_simulation.printlog("Creating README file.", level="info")
    eic_simulation.setup_readme()
    eic_simulation.printlog("Simulation process completed.", level="info")
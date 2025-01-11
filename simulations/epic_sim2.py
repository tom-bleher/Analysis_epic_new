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
        self.file_type: str = ""
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
            'file_type',
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
        Initialize the logger after settings are loaded.
        """
        self.logger = logging.getLogger("main_logger")
        self.logger.setLevel(logging.DEBUG)
        
        # Clear any existing handlers
        self.logger.handlers = []
        
        # Add file handler - now using backup_path
        self.overview_log_path = os.path.join(self.backup_path, "overview.log")
        file_handler = logging.FileHandler(self.overview_log_path, mode="w")
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(logging.Formatter("%(asctime)s - %(levelname)s - %(message)s"))
        self.logger.addHandler(file_handler)
        
        # Add console handler if enabled
        if self.console_logging:
            console_handler = logging.StreamHandler()
            console_handler.setLevel(logging.DEBUG)
            console_handler.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
            self.logger.addHandler(console_handler)
            print("Console logging enabled.")  # Direct print since logger might not be ready

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
            required_keys = ["px_pairs", "num_particles", "det_path", "file_type", "hepmc_path", "reconstruct", "sif_path"]
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
                console_handler.setLevel(logging.DEBUG)  # show all levels in the console
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

    def init_vars(
        self
        ) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """
        self.printlog("Initializing variables.", level="info")
        # get energy hepmcs from created
        self.energies = [file for file in (os.listdir(self.hepmc_path)) if self.file_type in file]
        self.printlog(f"Found energies: {self.energies}", level="info")

        # default if user does not provide JSON
        self.def_set_dict = {
            "px_pairs": [[2.0, 0.1]], # add more pixel pairs
            "num_particles": 100,
            "eic_shell_path": "/data/tomble/eic",
            "det_path": "/data/tomble/eic/epic", # sourced detector
            "file_type": "beamEffectsElectrons", 
            "hepmc_path": "/data/tomble/Analysis_epic_new/genFiles/results",
  
            "sim_out_path": "",
            "det_ip6_path": "",
            "console_logging": "True" # main settings pointer file of detector (should match sourced)
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
        Create simulation dictionary holding relevant parameters
        """
        self.printlog(f"Creating simulation dictionary for {curr_px_dx}x{curr_px_dy}.", level="info")
        # initialize dict to hold parameters for one simulation
        single_sim_dict = {}
        px_key = f"{curr_px_dx}x{curr_px_dy}"

        # Log all energies being processed
        self.printlog(f"Processing energies: {self.energies}", level="info")

        # populate dict entry with all simulation-relevant information
        single_sim_dict[px_key] = {
            "sim_det_path": curr_sim_det_path,
            "sim_compact_path": curr_sim_det_path + "/install/share/epic/compact",
            "sim_ip6_path": curr_sim_det_path + "/install/share/epic/epic_ip6_extended.xml",
            "sim_shell_path": f"{curr_sim_det_path}/install/bin/thisepic.sh",
        }

        # populate commands and gather recon output paths
        ddsim_cmds = []
        recon_cmds = []
        self.recon_out_paths = [] if self.reconstruct else None

        for energy in self.energies:
            self.printlog(f"Generating command for energy file: {energy}", level="debug")
            ddsim_cmd, output_file = self.get_ddsim_cmd(
                curr_sim_path, single_sim_dict[px_key]["sim_ip6_path"], energy
            )
            ddsim_cmds.append(ddsim_cmd)
            self.printlog(f"Generated ddsim command: {ddsim_cmd}", level="debug")

            if self.reconstruct:
                self.recon_out_paths.append(output_file)
                recon_cmd = self.get_recon_cmd(curr_sim_path, output_file, curr_sim_det_path)
                recon_cmds.append(recon_cmd)
                self.printlog(f"Generated recon command for {energy}: {recon_cmds[-1]}", level="debug")

        # Log final command counts
        self.printlog(f"Generated {len(ddsim_cmds)} ddsim commands", level="info")
        if self.reconstruct:
            self.printlog(f"Generated {len(recon_cmds)} reconstruction commands", level="info")

        # assign to dictionary
        single_sim_dict[px_key]["ddsim_cmds"] = ddsim_cmds
        if self.reconstruct:
            single_sim_dict[px_key]["recon_cmds"] = recon_cmds

        return single_sim_dict

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

    def get_ddsim_cmd(
        self, 
        curr_sim_path, 
        curr_sim_det_ip6_path, 
        energy
        ) -> Tuple[str, str]:
        """
        Generate ddsim command.
        """
        self.printlog(f"Generating ddsim command for energy {energy}.", level="info")
        inFile = os.path.join(self.hepmc_path, energy)
        # Remove extension to get the file_num
        file_num = os.path.splitext(energy)[0]
        output_file = os.path.join(curr_sim_path, f"output_{file_num}edm4hep.root")
        cmd = f"ddsim --inputFiles {inFile} --outputFile {output_file} --compactFile {curr_sim_det_ip6_path} -N {self.num_particles}"
        self.printlog(f"Generated ddsim command: {cmd}", level="info")
        return cmd, output_file

    def get_recon_cmd(self, curr_sim_path: str, sim_file: str, detector_path: str) -> dict:
        """
        Generate reconstruction command parameters
        """
        self.printlog(f"Generating recon parameters for file {sim_file}.", level="info")
        
        # Get base name of input file
        base_name = os.path.basename(sim_file)
        # Create standardized output name
        output_name = f"recon_{base_name}"
        
        # Create standardized output path - remove extra recon folder
        output_file = os.path.join(curr_sim_path, "recon", output_name)
        
        # Return dictionary with all needed paths
        return {
            'input_file': sim_file,
            'output_file': output_file,
            'detector_path': detector_path
        }

    def exec_sim(self) -> None:
        """
        Execute all simulations in parallel using ProcessPoolExecutor.
        Reconstruction is now handled in the same process via run_sim.sh
        """
        self.printlog("Executing simulations in parallel.", level="info")
        max_workers = os.cpu_count()
        
        try:
            with ProcessPoolExecutor(max_workers=max_workers) as executor:
                futures = []
                
                # Submit simulation jobs (reconstruction is handled within run_sim.sh)
                for px_key, sim_config in self.sim_dict.items():
                    detector_path = sim_config['sim_det_path']
                    shell_path = sim_config['sim_shell_path']
                    
                    for sim_cmd in sim_config['ddsim_cmds']:
                        futures.append(
                            executor.submit(
                                self.run_single_sim,
                                sim_cmd=sim_cmd,
                                detector_path=detector_path,
                                shell_path=shell_path,
                                px_key=px_key
                            )
                        )
                
                # Process all results
                for future in as_completed(futures):
                    try:
                        result = future.result()
                        self.printlog(f"Completed simulation/reconstruction: {result}", level="info")
                    except Exception as e:
                        self.printlog(f"Process failed: {e}", level="error")
                        raise
                    
        finally:
            # Always create README
            try:
                self.setup_readme()
                self.printlog("README file created successfully.", level="info")
            except Exception as e:
                self.printlog(f"Failed to create README: {e}", level="error")
            """
            # Merge reconstruction outputs if they exist
            if self.reconstruct:
                try:
                    self.merge_recon_out()
                    self.printlog("Reconstruction outputs merged successfully.", level="info")
                except Exception as e:
                    self.printlog(f"Failed to merge reconstruction outputs: {e}", level="error")
            """
    def exec_simv2(self) -> None:
        """
        Enhanced parallel execution with better tracking of energy levels.
        """
        self.printlog("Starting enhanced parallel execution.", level="info")
        max_workers = min(os.cpu_count(), 64)  # limit max workers to prevent overload
        
        # Create dictionaries to track task status and results
        task_status = {}
        failed_tasks = []
        completed_sims = set()
        pending_recon = []
        
        # Log all detected energy files at the start
        self.printlog(f"Found energy files: {self.energies}", level="info")
        
        try:
            with ProcessPoolExecutor(max_workers=max_workers) as executor:
                # First submit and complete all simulation tasks
                sim_futures = []
                
                # Submit all simulation tasks with better tracking
                for px_key, sim_config in self.sim_dict.items():
                    detector_path = sim_config['sim_det_path']
                    shell_path = sim_config['sim_shell_path']
                    
                    self.printlog(f"Processing pixel pair {px_key}", level="info")
                    self.printlog(f"Number of ddsim commands: {len(sim_config['ddsim_cmds'])}", level="info")
                    
                    # Log each command being submitted
                    for sim_cmd in sim_config['ddsim_cmds']:
                        energy_match = re.search(rf'{self.file_type}_(\d+)', sim_cmd)
                        if not energy_match:
                            self.printlog(f"Could not extract energy from command: {sim_cmd}", level="warning")
                            continue
                            
                        energy = energy_match.group(1)
                        task_id = f"{px_key}_{energy}"
                        
                        self.printlog(f"Submitting simulation task for energy {energy}", level="info")
                        
                        future = executor.submit(
                            self.run_single_sim,
                            sim_cmd=sim_cmd,
                            detector_path=detector_path,
                            shell_path=shell_path,
                            px_key=px_key
                        )
                        sim_futures.append((future, task_id, px_key, energy))
                
                # Wait for all simulations to complete before starting reconstructions
                for future, task_id, px_key, energy in sim_futures:
                    try:
                        result = future.result()
                        self.printlog(f"Simulation completed: {task_id} - {result}", level="info")
                        completed_sims.add((px_key, energy))
                        task_status[task_id] = {'status': 'completed', 'result': result}
                        
                        # Add to pending reconstruction queue
                        if self.reconstruct:
                            pending_recon.append((px_key, energy))
                            
                    except Exception as e:
                        self.printlog(f"Simulation failed for {task_id}: {str(e)}", level="error")
                        failed_tasks.append((task_id, str(e)))
                        task_status[task_id] = {'status': 'failed', 'error': str(e)}
                
                # Now process all reconstructions after simulations are complete
                if self.reconstruct and pending_recon:
                    recon_futures = []
                    
                    for px_key, energy in pending_recon:
                        recon_cmd = self.get_recon_cmd_for_energy(px_key, energy)
                        if recon_cmd:
                            self.printlog(f"Starting reconstruction for {px_key} energy {energy}", level="info")
                            future = executor.submit(
                                self.run_reconstruction,
                                recon_cmd=recon_cmd,
                                px_key=px_key
                            )
                            recon_futures.append((future, f"{px_key}_{energy}_recon", px_key, energy))
                    
                    # Wait for all reconstructions to complete
                    for future, task_id, px_key, energy in recon_futures:
                        try:
                            result = future.result()
                            # Verify the reconstruction output file exists and has content
                            recon_file = os.path.join(self.backup_path, f"{px_key}px", "recon", 
                                                    f"recon_output_beamEffectsElectrons_{energy}edm4hep.root")
                            if not os.path.exists(recon_file) or os.path.getsize(recon_file) < 1000:
                                raise Exception(f"Reconstruction failed - Invalid or missing output file: {recon_file}")
                                
                            self.printlog(f"Reconstruction completed for {task_id}: {result}", level="info")
                            task_status[task_id] = {'status': 'completed', 'result': result}
                        except Exception as e:
                            self.printlog(f"Reconstruction failed for {task_id}: {str(e)}", level="error")
                            failed_tasks.append((task_id, str(e)))
                            task_status[task_id] = {'status': 'failed', 'error': str(e)}
        
        except Exception as e:
            self.printlog(f"Execution failed with error: {str(e)}", level="error")
            failed_tasks.append(("global", str(e)))
            raise
            
        finally:
            # Log execution summary
            self.log_execution_summary(task_status, failed_tasks)
            self.create_execution_report(task_status)
            self.printlog("Execution completed.", level="info")

    def get_recon_cmd_for_energy(self, px_key: str, energy: str) -> dict:
        """
        Generate reconstruction command for specific pixel key and energy level.
        """
        sim_file = os.path.join(
            self.backup_path,
            f"{px_key}px",
            f"output_{self.file_type}_{energy}edm4hep.root"
        )
        
        # Get the detector path from sim_dict
        detector_path = self.sim_dict[px_key]['sim_det_path']
        
        if os.path.exists(sim_file):
            return self.get_recon_cmd(
                curr_sim_path=os.path.dirname(sim_file),
                sim_file=sim_file,
                detector_path=detector_path
            )
        return None

    def create_execution_report(self, task_status: dict) -> None:
        """
        Create a detailed execution report.
        """
        report_path = os.path.join(self.backup_path, "execution_report.txt")
        with open(report_path, 'w') as f:
            f.write("Execution Report\n")
            f.write("================\n\n")
            
            # Group tasks by pixel pair
            tasks_by_pixel = {}
            for task_id, status in task_status.items():
                px_key = task_id.split('_')[0]
                if px_key not in tasks_by_pixel:
                    tasks_by_pixel[px_key] = []
                tasks_by_pixel[px_key].append((task_id, status))
            
            # Write detailed status for each pixel pair
            for px_key, tasks in tasks_by_pixel.items():
                f.write(f"\nPixel Pair: {px_key}\n")
                f.write("-" * (len(px_key) + 12) + "\n")
                
                for task_id, status in tasks:
                    f.write(f"Task: {task_id}\n")
                    f.write(f"Status: {status['status']}\n")
                    if 'error' in status:
                        f.write(f"Error: {status['error']}\n")
                    f.write("\n")

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

    def run_single_sim(self, sim_cmd: str, detector_path: str, shell_path: str, px_key: str) -> str:
        """
        Execute a single simulation and optional reconstruction by calling run_sim.sh script.
        """
        subprocess_logger, log_file = self.setup_subprocess_logger(sim_cmd, px_key)
        
        try:
            # Validate paths
            if not os.path.exists(detector_path):
                raise FileNotFoundError(f"Detector path not found: {detector_path}")
            if not os.path.exists(self.sif_path):
                raise FileNotFoundError(f"Singularity image not found: {self.sif_path}")
            
            # Get absolute paths
            abs_detector_path = os.path.abspath(detector_path)
            abs_detector_parent = os.path.dirname(abs_detector_path)
            
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
                str(self.reconstruct).lower(),  # Convert boolean to string "true" or "false"
                self.plugin_path if self.reconstruct else ""
            ]
            
            # Execute the script
            subprocess_logger.info(f"Starting simulation with command: {' '.join(cmd)}")
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
                "undefined reference", "Segmentation fault"
            ]
            
            # Monitor output in real-time with better error detection
            for line in iter(process.stdout.readline, ''):
                line = line.strip()
                subprocess_logger.info(line)
            
            for line in iter(process.stderr.readline, ''):
                line = line.strip()
                # Check if line contains actual error patterns
                if any(pattern in line for pattern in error_patterns):
                    subprocess_logger.error(line)
                else:
                    # Treat build output and warnings as info
                    subprocess_logger.info(line)
            
            process.wait()
            if process.returncode != 0:
                raise RuntimeError(f"Simulation failed with return code {process.returncode}")
            
            return f"Successfully completed simulation for {px_key}"
            
        except Exception as e:
            subprocess_logger.error(f"Error in simulation: {str(e)}")
            raise

    def setup_subprocess_logger(self, sim_cmd: str, px_key: str) -> Tuple[logging.Logger, str]:
        """
        Create a separate logger for each subprocess with its own log file in the pixel folder.
        """
        logger_name = f"subprocess_{hash(sim_cmd)}_{os.getpid()}"
        logger = logging.getLogger(logger_name)
        
        if not logger.hasHandlers():
            logger.setLevel(logging.DEBUG)
            
            # Create log file in the pixel-specific directory within backup_path
            px_path = os.path.join(self.backup_path, f"{px_key}px")
            os.makedirs(px_path, exist_ok=True)
            
            log_file = os.path.join(
                px_path,
                f"subprocess.log"
            )
            
            file_handler = logging.FileHandler(log_file)
            file_handler.setFormatter(logging.Formatter(
                '%(asctime)s - Process %(process)d - %(levelname)s - %(message)s'
            ))
            logger.addHandler(file_handler)
            
            if self.console_logging:
                console_handler = logging.StreamHandler()
                console_handler.setFormatter(logging.Formatter(
                    '%(levelname)s - Process %(process)d - %(message)s'
                ))
                logger.addHandler(console_handler)
        
        return logger, log_file

    def run_reconstruction(self, recon_cmd: dict, px_key: str) -> str:
        """
        Execute reconstruction using run_sim.sh script within Singularity container.
        """
        max_retries = 3
        retry_count = 0
        subprocess_logger = logging.getLogger('subprocess')
        
        while retry_count < max_retries:
            try:
                # Create reconstruction directory if it doesn't exist
                recon_dir = os.path.dirname(recon_cmd['output_file'])
                os.makedirs(recon_dir, exist_ok=True)
                
                # Get paths
                detector_path = recon_cmd['detector_path']
                detector_parent = os.path.dirname(detector_path)
                
                # Prepare reconstruction command for run_sim.sh
                script_path = os.path.join(os.path.dirname(__file__), "run_sim.sh")
                
                # Build the eicrecon command that will run inside Singularity
                eicrecon_cmd = (
                    f"eicrecon "
                    f"-Pplugins={self.plugin_path} "
                    f"-Pdd4hep_dir={detector_path} "
                    f"{recon_cmd['input_file']} "
                    f"-o {recon_cmd['output_file']}"
                )
                
                # Full command to run reconstruction in Singularity
                cmd = [
                    script_path,
                    detector_path,
                    detector_parent,
                    self.execution_path,
                    self.sim_out_path,
                    self.sif_path,
                    eicrecon_cmd,
                    "true",  # reconstruction flag
                    self.plugin_path
                ]
                
                subprocess_logger.info(f"Running reconstruction command: {' '.join(cmd)}")
                
                # Execute reconstruction
                result = subprocess.run(
                    cmd,
                    check=True,
                    capture_output=True,
                    text=True,
                    env=os.environ.copy()
                )
                
                # Log output
                if result.stdout:
                    subprocess_logger.info(result.stdout)
                if result.stderr:
                    subprocess_logger.error(result.stderr)
                
                # Add a delay before checking file
                time.sleep(2)
                
                # Verify output file exists and has content with more detailed logging
                if not os.path.exists(recon_cmd['output_file']):
                    raise Exception(f"Output file not created: {recon_cmd['output_file']}")
                    
                file_size = os.path.getsize(recon_cmd['output_file'])
                if file_size < 1000:  # Minimum expected file size in bytes
                    raise Exception(f"Output file too small ({file_size} bytes): {recon_cmd['output_file']}")
                    
                subprocess_logger.info(f"Reconstruction successful - Output file size: {file_size} bytes")
                subprocess_logger.info(f"Output file details:")
                subprocess_logger.info(subprocess.check_output(['ls', '-l', recon_cmd['output_file']]).decode())
                return "Reconstruction completed successfully"
                
            except Exception as e:
                retry_count += 1
                subprocess_logger.error(f"Error in reconstruction (attempt {retry_count}/{max_retries}): {str(e)}")
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
            # get BH value from the function
            self.BH_val = self.get_BH_val()
            self.printlog(f"Retrieved BH value: {self.BH_val}", level="info")
        
            # get energy levels from file names of genEvents
            self.energy_vals = [
                '.'.join(file.split('_')[1].split('.', 2)[:2]) 
                for file in self.energies 
            ]
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
                self.printlog(f"Added py_file: {self.file_type}", level="info")

                # write each key-value pair from the settings_dict
                for key, value in self.settings_dict.items():
                    file.write(f"{key}: {value}\n")
                    self.printlog(f"Added setting: {key}: {value}")

                file.write(f"file_type: {self.file_type}\n")
                self.printlog(f"Added file_type: {self.file_type}", level="info")
                
                file.write(f"Number of Particles: {self.num_particles}\n")
                self.printlog(f"Added num_particles: {self.num_particles}", level="info")
                
                file.write(f"Pixel Value Pairs: {self.px_pairs}\n")
                self.printlog(f"Added px_pairs: {self.px_pairs}", level="info")
                
                file.write(f"BH: {self.BH_val}\n")
                self.printlog(f"Added BH: {self.BH_val}", level="info")
                
                file.write(f"Energy Levels: {self.energy_vals}\n")
                self.printlog(f"Added energy levels: {self.energy_vals}", level="info")

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

    def get_BH_val(
        self
        ) -> None:
        """
        Retrieve the BH value from the createGenFiles.py file.
        """
        self.printlog("Retrieving BH value from createGenFiles.py.", level="info")
        # open the path storing the createGenFiles.py file
        with open(self.GenFiles_path, 'r') as file:
            content = file.read()
            
        # use a regex to find the line where BH is defined
        match = re.search(r'BH\s*=\s*(.+)', content)
        
        # if we found BH in the file, we return the value
        if match:
            value = match.group(1).strip()
            self.printlog(f"Retrieved BH value: {value}", level="info")
            return value
        else:
            self.printlog("Could not find a value for 'BH' in the content of the file.", level="error")
            raise ValueError

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
            
            # Find all reconstruction outputs with updated pattern
            recon_files = []
            for root, _, files in os.walk(recon_dir):
                self.printlog(f"Searching in: {root}", level="debug")
                for file in files:
                    if file.startswith("recon_") and file.endswith(".root"):
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

    # prepare the simulation based on settings (now working directly in backup location)
    eic_simulation.printlog("Preparing simulation based on settings.", level="info")
    eic_simulation.prep_sim()

    # execute the simulation and reconstruction in parallel    eic_simulation.printlog("Executing simulation and reconstruction in parallel.", level="info")
    eic_simulation.exec_simv2()  # Use the new execution function

    # Only merge if reconstruction was successful
    if eic_simulation.reconstruct:
        eic_simulation.printlog("Merging reconstruction outputs.", level="info")
        #eic_simulation.merge_recon_out()

    # Create README directly instead of calling mk_sim_backup
    eic_simulation.printlog("Creating README file.", level="info")
    eic_simulation.setup_readme()
    eic_simulation.printlog("Simulation process completed.", level="info")
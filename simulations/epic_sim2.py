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
            if missing_settings:
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

    def create_sim_dict(
        self, 
        curr_sim_path: str, 
        curr_sim_det_path: str, 
        curr_px_dx: float, 
        curr_px_dy: float
        ) -> Dict[str, dict[str, str]]:
        """
        Create simulation dictionary holding relavent parameters
        """
        self.printlog(f"Creating simulation dictionary for {curr_px_dx}x{curr_px_dy}.", level="info")
        # initilize dict to hold parameters for one simulation
        single_sim_dict = {}
        px_key = f"{curr_px_dx}x{curr_px_dy}"

        # populate dict entry with all simulation-relavent information
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
            ddsim_cmd, output_file = self.get_ddsim_cmd(
                curr_sim_path, single_sim_dict[px_key]["sim_ip6_path"], energy
            )
            ddsim_cmds.append(ddsim_cmd)
            self.printlog(f"Generated ddsim command: {ddsim_cmd}", level="debug")

            if self.reconstruct:
                self.recon_out_paths.append(output_file)
                recon_cmds.append(self.get_recon_cmd(curr_sim_path, output_file))
                self.printlog(f"Generated recon command for {energy}: {recon_cmds[-1]}", level="debug")

        # assign to dictionary
        single_sim_dict[px_key]["ddsim_cmds"] = ddsim_cmds
        if self.reconstruct:
            single_sim_dict[px_key]["recon_cmds"] = recon_cmds

        # return the dict for the sim
        self.printlog(f"Created simulation dictionary: {json.dumps(single_sim_dict, indent=2)}", level="info")
        return single_sim_dict

    def copy_epic(
        self, 
        curr_sim_path
        ) -> str:
        """
        Copy uncompiled detector to simulation directory.
        """
        self.printlog(f"Copying epic detector to {curr_sim_path}.", level="info")
        try:
            det_name = self.det_path.split('/')[-1]
            dest_path = os.path.join(curr_sim_path, det_name)
            
            # Copy fresh, uncompiled detector
            if os.path.exists(dest_path):
                shutil.rmtree(dest_path)
            shutil.copytree(self.det_path, dest_path, symlinks=True)
            
            # Clean any existing build artifacts
            build_path = os.path.join(dest_path, 'build')
            if os.path.exists(build_path):
                shutil.rmtree(build_path)
            os.makedirs(build_path)
            
            self.printlog(f"Copied fresh detector to {dest_path}.", level="info")
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
        match = re.search(r"\d+\.+\d\.", inFile)
        file_num = match.group() if match else energy.split("_")[1].split(".")[0]
        
        output_file = os.path.join(curr_sim_path, f"output_{file_num}edm4hep.root")
        cmd = f"ddsim --inputFiles {inFile} --outputFile {output_file} --compactFile {curr_sim_det_ip6_path} -N {self.num_particles}"
        self.printlog(f"Generated ddsim command: {cmd}", level="info")
        return cmd, output_file

    def get_recon_cmd(
        self, 
        curr_sim_path, 
        file
        ) -> str:
        """
        Method to run EIC recon on created simulation files
        """
        self.printlog(f"Generating recon command for file {file}.", level="info")
        # in the backup path, loop over pixel folders to find output root files
        inFile = os.path.join(curr_sim_path, file)
        match = re.search("\d+\.+\d\.", inFile)
        file_num = match.group() if match else file.split('_')[1][:2]

        output_file = f'{curr_sim_path}/eicrecon_{file_num}.root'
        cmd = f"eicrecon -Pplugins=analyzeLumiHits -Phistsfile={output_file} {inFile}"
        self.printlog(f"Generated recon command: {cmd}", level="info")
        return cmd

    def exec_sim(self) -> None:
        """
        Execute all simulations in parallel using ProcessPoolExecutor.
        """
        self.printlog("Executing simulations in parallel.", level="info")
        max_workers = os.cpu_count()  
        with ProcessPoolExecutor(max_workers=max_workers) as executor:
            futures = []
            
            # Submit jobs for each pixel configuration
            for px_key, sim_config in self.sim_dict.items():
                detector_path = sim_config['sim_det_path']
                shell_path = sim_config['sim_shell_path']
                
                # Submit each simulation command as a separate job
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
            
            # Process results as they complete
            for future in as_completed(futures):
                try:
                    result = future.result()
                    self.printlog(f"Completed simulation: {result}", level="info")
                except Exception as e:
                    self.printlog(f"Simulation failed: {e}", level="error")
                    raise

    def run_single_sim(self, sim_cmd: str, detector_path: str, shell_path: str, px_key: str) -> str:
        """
        Enhanced single simulation execution with better error handling and validation.
        """
        subprocess_logger, log_file = self.setup_subprocess_logger(sim_cmd, px_key)
        script_path = None  # Initialize script_path outside the try block
        
        try:
            # Validate paths
            if not os.path.exists(detector_path):
                raise FileNotFoundError(f"Detector path not found: {detector_path}")
            if not os.path.exists(self.sif_path):
                raise FileNotFoundError(f"Singularity image not found: {self.sif_path}")
            
            # Get absolute paths
            abs_detector_path = os.path.abspath(detector_path)
            abs_detector_parent = os.path.dirname(abs_detector_path)
            
            # Create simulation script that builds detector first
            if self.reconstruct:
                script_content = [
                    "#!/bin/bash",
                    "set -e",  # Exit on any error
                    "set -o pipefail",  # Pipe failures are treated as errors
                    
                    # Mount all necessary directories with their absolute paths
                    f"singularity exec --containall \\\n"
                    f"    --bind {abs_detector_parent}:{abs_detector_parent} \\\n"
                    f"    --bind {self.execution_path}:{self.execution_path} \\\n"
                    f"    --bind {self.sim_out_path}:{self.sim_out_path} \\\n"
                    f"    {self.sif_path} /bin/bash << 'EOF'",
                    
                    # Use absolute paths inside container
                    f"cd {abs_detector_path}",
                    "mkdir -p build",
                    "mkdir -p install",
                    "rm -rf build/*",
                    "cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install",
                    "cmake --build build --target install -- -j$(nproc)",
                    
                    # Verify shell script exists
                    "if [ ! -f install/bin/thisepic.sh ]; then",
                    "    echo 'Error: thisepic.sh not found after build'",
                    "    exit 1",
                    "fi",
                    
                    # Source environment with absolute path
                    f"source {abs_detector_path}/install/bin/thisepic.sh",
                    
                    # Run simulation with actual paths
                    f"{sim_cmd}",
                    
                    # Run reconstruction
                    f"echo 'Running reconstruction for {px_key}'",
                    f"eicrecon -Pplugins=analyzeLumiHits -Phistsfile={abs_detector_path}/install/share/epic/{px_key}_recon.root {self.sim_out_path}/{px_key}/output_{px_key}.root",
                    "EOF"  # Exit singularity
                ]
            else:
                script_content = [
                    "#!/bin/bash",
                    "set -e",
                    "set -o pipefail",
                    
                    # Mount all necessary directories with their absolute paths
                    f"singularity exec --containall \\\n"
                    f"    --bind {abs_detector_parent}:{abs_detector_parent} \\\n"
                    f"    --bind {self.execution_path}:{self.execution_path} \\\n"
                    f"    --bind {self.sim_out_path}:{self.sim_out_path} \\\n"
                    f"    {self.sif_path} /bin/bash << 'EOF'",
                    
                    # Use absolute paths inside container
                    f"cd {abs_detector_path}",
                    "mkdir -p build",
                    "mkdir -p install",
                    "rm -rf build/*",
                    "cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install",
                    "cmake --build build --target install -- -j$(nproc)",
                    
                    # Verify shell script exists
                    "if [ ! -f install/bin/thisepic.sh ]; then",
                    "    echo 'Error: thisepic.sh not found after build'",
                    "    exit 1",
                    "fi",
                    
                    # Source environment with absolute path
                    f"source {abs_detector_path}/install/bin/thisepic.sh",
                    
                    # Run simulation with actual paths
                    f"{sim_cmd}",
                    
                    "EOF"  # Exit singularity
                ]
                
            # Write script to temporary file using absolute path
            script_path = os.path.join(abs_detector_path, f"sim_{os.getpid()}.sh")
            with open(script_path, 'w') as f:
                f.write('\n'.join(script_content))
            os.chmod(script_path, 0o755)
            
            # Execute the script
            subprocess_logger.info(f"Starting simulation with command: {sim_cmd}")
            process = subprocess.Popen(
                [script_path],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            
            # Monitor output in real-time
            for line in iter(process.stdout.readline, ''):
                subprocess_logger.info(line.strip())
                
            # Improved stderr handling with proper message categorization
            for line in iter(process.stderr.readline, ''):
                line = line.strip()
                # Check if the line contains any variation of "info" (case insensitive)
                if any(info_variant in line.lower() for info_variant in ['info', 'Info', 'INFO']):
                    subprocess_logger.info(line)
                else:
                    subprocess_logger.error(line)
                
            process.wait()
            if process.returncode != 0:
                raise RuntimeError(f"Simulation failed with return code {process.returncode}")
            
            return f"Successfully completed simulation for {px_key}"
            
        except Exception as e:
            subprocess_logger.error(f"Error in simulation: {str(e)}")
            raise
        finally:
            # Clean up script file if it was created
            if script_path and os.path.exists(script_path):
                try:
                    os.remove(script_path)
                except Exception as e:
                    subprocess_logger.error(f"Failed to remove script file: {e}")

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
                f"subprocess_{datetime.now().strftime('%Y%m%d_%H%M%S')}_{os.getpid()}.log"
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
            self.photon_energy_vals = [
                '.'.join(file.split('_')[1].split('.', 2)[:2]) 
                for file in self.energies 
            ]
            self.printlog(f"Extracted photon energy levels: {self.photon_energy_vals}", level="info")
        
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
                
                file.write(f"Energy Levels: {self.photon_energy_vals}\n")
                self.printlog(f"Added photon energy levels: {self.photon_energy_vals}", level="info")

                file.write(f"DETECTOR_PATH: {detector_path}\n")
                self.printlog(f"Added DETECTOR_PATH: {detector_path}", level="info")

                file.write(f"Executed from Singularity: {in_singularity}\n")
                if in_singularity:
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
        Checks if the reconstruction output at the given path is successful. Reconstruction is considered successful if the file size is >= 1000 bytes.
        
        Args:
            recon_path (str): Path to the reconstruction output file.
        
        Returns:
            bool: True if successful, False otherwise.
        """
        self.printlog(f"Checking reconstruction output size for {recon_path}.", level="info")
        try:
            # use subprocess to safely execute the command and capture the output
            result = subprocess.run(
                ["stat", "-c", "%s", recon_path],
                capture_output=True,
                text=True,
                check=True
            )
            filesize = int(result.stdout.strip())
            self.printlog(f"File size for {recon_path}: {filesize} bytes.", level="debug")
            if filesize < 1000:  # less than 1000 bytes indicates failure
                self.printlog(f"Failed reconstruction for file at path {recon_path}.", level="error")
                return False
            return True
        except subprocess.CalledProcessError as e:
            self.printlog(f"Checking recon output size failed: {e}", level="error")
            raise RuntimeError(f"Failed to check file size for {recon_path}") from e
        except ValueError:
            self.printlog(f"Invalid file size value returned for {recon_path}", level="error")
            raise
            
    def merge_recon_out(self):
        """
        This method merges all the different root files
        """
        self.printlog("Merging reconstruction output files.", level="info")
        all_recon_out_paths = []
        for px_key, sim_data in self.sim_dict.items():
            if "recon_cmds" in sim_data:
                all_recon_out_paths.extend(self.recon_out_paths)
        self.printlog(f"Reconstruction output paths: {all_recon_out_paths}", level="debug")

        os.system(f"hadd {self.backup_path}/eicrecon_MergedOutput.root {' '.join(all_recon_out_paths)}")
        self.printlog("Merged reconstruction output files.", level="info")

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

    # execute the simulation in parallel
    eic_simulation.printlog("Executing simulation in parallel.", level="info")
    eic_simulation.exec_sim()

    """ Reconstruction """
    if eic_simulation.reconstruct:
        eic_simulation.printlog("Reconstruction is enabled. Merging reconstruction outputs.", level="info")
        eic_simulation.merge_recon_out()

    # Create README directly instead of calling mk_sim_backup
    eic_simulation.printlog("Creating README file.", level="info")
    eic_simulation.setup_readme()
    eic_simulation.printlog("Simulation process completed.", level="info")

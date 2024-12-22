"""
Created on Fri Sep  6 15:33:11 2024

@author: tombleher
"""
import os
import shutil
from datetime import datetime
import sys
import re
import json
import xml.etree.ElementTree as ET
import subprocess
import multiprocessing
import logging
from typing import Dict, List, Tuple
import concurrent
from concurrent.futures import ProcessPoolExecutor, as_completed

class HandleEIC(object):

    def __init__(
        self,
        ) -> None:
        
        # init internal variables
        self.energies: List[str] = []
        self.sim_dict: Dict[str, Dict[str, str]] = {}
        self.settings_path: str = "simulation_settings.json"
        self.execution_path: str = os.getcwd()
        self.backup_path: str = ""
        
        # init vars to be populated by the JSON file
        self.px_pairs: List[Tuple[float, float]] = []
        self.num_particles: int = 0
        self.eic_shell_path: str = ""
        self.det_path: str = ""
        self.file_type: str = ""
        self.hepmc_path: str = ""
        self.sim_out_path: str = ""
        self.det_ip6_path: str = ""
        self.program_prints = True

        # initialize logging
        self.self.logger = self.setup_logger("main_logger", "logging.log")
        self.logger.info("Initialized HandleEIC class.")

    def printlog(self, message: str, level: str = "info") -> None:
        """
        Log a message and optionally print it to the console.
        :param message: Message to log and optionally print.
        :param level: Log level ('info', 'warning', 'error', etc.).
        """
        # Log the message at the appropriate level
        if level.lower() == "info":
            self.logger.info(message)
        elif level.lower() == "warning":
            self.logger.warning(message)
        elif level.lower() == "error":
            self.logger.error(message)
        elif level.lower() == "debug":
            self.logger.debug(message)
        else:
            self.logger.info(message)  # Default to info level

        # Optionally print the message
        if self.program_prints:
            print(f"{level.upper()}: {message}")

    def setup_logger(self, name: str, log_file: str, level=logging.INFO) -> logging.Logger:
        """
        Set up a logger with file and console handlers.
        """
        logger = logging.getLogger(name)
        logger.setLevel(level)
        
        # File handler
        file_handler = logging.FileHandler(log_file, mode="w")
        file_handler.setFormatter(logging.Formatter("%(asctime)s - %(levelname)s - %(message)s"))
        logger.addHandler(file_handler)
        
        return logger

    def setup_settings(
        self
        ) -> None:
        """
        Load simulation settings from a JSON file. If the file is missing or invalid,
        create a default settings file and raise an error for the user to edit.
        """
        try:
            if os.path.exists(self.settings_path):
                with open(self.settings_path, 'r') as file:
                    self.settings_dict = json.load(file)
            else:
                self.printlog(f"Settings file not found at {self.settings_path}.")
                raise FileNotFoundError(f"Settings file not found at {self.settings_path}.")

            required_keys = ["px_pairs", "num_particles", "det_path", "file_type", "hepmc_path"]
            for key in required_keys:
                if key not in self.settings_dict or not self.settings_dict[key]:
                    self.printlog(f"Missing or empty key: {key} in settings.")
                    raise ValueError(f"Missing or empty key: {key} in settings.")

            # set attributes dynamically
            for key, value in self.settings_dict.items():
                setattr(self, key, value)

        except FileNotFoundError:
            logging.info("Failed to load settings, creating default configuration.")
            with open(self.settings_path, 'w') as file:
                json.dump(self.def_set_dict, file, indent=4)
            raise RuntimeError(f"Settings JSON created at {self.settings_path}. Edit and rerun.") from e            

        except json.JSONDecodeError as e:
            self.printlog(f"Failed to parse settings file: {e}. Check for JSON formatting issues.")
            raise ValueError("Invalid JSON format in settings file. Fix the file or delete it to recreate.")

        except Exception as e:
            self.printlog(f"Unexpected error while loading settings: {e}", level="error")
            raise RuntimeError("Failed to load settings due to an unexpected error.") from e

    def init_paths(
        self
        ) -> None:

        self.settings_path = "simulation_settings.json"

        if not os.path.isdir(self.sim_out_path) or self.sim_out_path == "":
            self.sim_out_path = self.execution_path + "/simEvents"
        else:
            self.sim_out_path = self.settings_dict.get("sim_out_path")
        os.makedirs(self.sim_out_path, exist_ok=True)

        if not os.path.exists(self.hepmc_path):
            raise ValueError(f"The specified hepmc path {self.hepmc_path} does not exist. Run GenSimFiles.py and link its output correctly.")

        # create the path where the simulation file backup will go
        self.backup_path = os.path.join(self.sim_out_path , datetime.now().strftime("%Y%m%d_%H%M%S"))
        self.GenFiles_path = os.path.join(self.execution_path, "createGenFiles.py")
        if not os.path.isfile(self.GenFiles_path):
            print("Did not find your GenFiles.py file")
            raise

    def init_vars(
        self
        ) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """

        # get energy hepmcs from created
        self.energies = [file for file in (os.listdir(self.hepmc_path)) if self.file_type in file]

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
            "program_prints": "True" # main settings pointer file of detector (should match sourced)
        }

    def prep_sim(
        self
        ) -> None:

        # loop over all requested detector changes and create specifics
        self.printlog(f"Starting simulation loop for px_pairs for init_specifics: {self.px_pairs}")
        for curr_px_dx, curr_px_dy in self.px_pairs:
            
            """ soft changes """
            # create respective px folder to hold simulation output and change-related objects
            curr_sim_path = os.path.join(self.sim_out_path, f"{curr_px_dx}x{curr_px_dy}px")
            os.makedirs(curr_sim_path, exist_ok=True)

            # copy epic folder to current pixel path
            curr_sim_det_path = self.copy_epic(curr_sim_path)

            """ hard changes """
            # rewrite detector's XMLs to hold current change for detector
            self.mod_detector_settings(curr_sim_det_path, curr_px_dx, curr_px_dy)
            
            """ gather simulation relavent details """
            # update the simulation dictionary for current requested change
            single_sim_dict = self.create_sim_dict(curr_sim_path, curr_sim_det_path, curr_px_dx, curr_px_dy)
            self.sim_dict.update(single_sim_dict)
        
    def create_sim_dict(
        self, 
        curr_sim_path: str, 
        curr_sim_det_path: str, 
        curr_px_dx: float, 
        curr_px_dy: float
        ) -> Dict[str, dict[str, str]]:
        """
        Create simulation dictionary holding relavent parameters
        "dx_dy"
            "epic"
            "compact"
            "ip6"
            "sh_file_path"
        """
        
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

        # populate px_ddsim_cmds according to requested energies in hepmc_path
        single_sim_dict[px_key]["px_ddsim_cmds"] = [self.get_ddsim_cmd(curr_sim_path, single_sim_dict[px_key]["sim_ip6_path"], energy) for energy in self.energies]

        # return the dict for the sim
        self.printlog(f"Created simulation dictionary: {json.dumps(self.sim_dict, indent=2)}")
        return single_sim_dict

    def copy_epic(self, curr_sim_path) -> str:
        """Copy epic to respective px folder for parameter reference."""
        try:
            det_name = self.det_path.split('/')[-1]
            dest_path = os.path.join(curr_sim_path, det_name)
            
            # create destination directory if it doesn't exist
            os.makedirs(dest_path, exist_ok=True)
            
            # use find to copy all files except .git
            os.system(f'find {self.det_path} -path {self.det_path}/.git -prune -o -exec cp -r {{}} {dest_path}/ \\;')
            
            return dest_path
        except Exception as e:
            logging.error(f"Failed to copy detector: {e}")
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
                        #logging.info(f"Updated {filepath} with pixel sizes dx={curr_px_dx}, dy={curr_px_dy}")
                    except Exception as e:
                        logging.error(f"Failed to modify {filepath}: {e}")
                        raise RuntimeError(f"Error in mod_detector_settings: {filepath}") from e

    def get_ddsim_cmd(
        self, 
        curr_sim_path, 
        curr_sim_det_ip6_path, 
        energy
        ) -> str:
        """
        Generate ddsim command.
        """
        inFile = os.path.join(self.hepmc_path, energy)
        match = re.search(r"\d+\.+\d\.", inFile)
        file_num = match.group() if match else energy.split("_")[1].split(".")[0]
        
        output_file = os.path.join(curr_sim_path, f"output_{file_num}edm4hep.root")
        cmd = f"ddsim --inputFiles {inFile} --outputFile {output_file} --compactFile {curr_sim_det_ip6_path} -N {self.num_particles}"
        self.printlog(f"Generated ddsim command: {cmd}")
        return cmd

    def exec_simv2(self) -> None:
        """
        Execute all simulations in parallel using multiprocessing and return results as they are completed.
        """
        # Prepare the run queue
        run_queue = [
            (sim_cmd, paths['sim_shell_path'], paths['sim_det_path'])
            for px_key, paths in self.sim_dict.items()
            for sim_cmd in paths['px_ddsim_cmds']
        ]

        with multiprocessing.Pool(processes=os.cpu_count()) as pool:
            pool.imap_unordered(self.run_cmd, run_queue)

    def exec_sim(self) -> None:
        """
        Execute the simulation for all entries in the simulation dictionary,
        ensuring recompilation and sourcing of the detector environment in each subprocess.
        """
        self.printlog("Preparing simulation commands and environments.")

        # prepare the run queue
        run_queue = [ 
            # sim_cmd has according pixel ip6 XML file
            (sim_cmd, paths['sim_shell_path'], paths['sim_det_path'])
            for px_key, paths in self.sim_dict.items()
            for sim_cmd in paths['px_ddsim_cmds']
         ]
                
        with ProcessPoolExecutor(max_workers=os.cpu_count()) as executor:
            future_to_cmd = {executor.submit(self.run_cmd, cmd): cmd for cmd in run_queue}
            for future in concurrent.futures.as_completed(future_to_cmd):
                cmd = future_to_cmd[future]
                try:
                    future.result()  # handle success
                    self.printlog(f"Simulation completed for command: {cmd[0]}")
                except Exception as e:
                    self.printlog(f"Simulation failed for command {cmd[0]}: {e}", level="error")

    def recompile_det_cmd(self, det_path: str, method: str) -> str:
        """
        Generates a recompile command for a given detector path and method.
        
        :param det_path: Path to the detector source code.
        :param method: Compilation method, either 'delete_build' or 'make_clean'.
        :return: The full recompile command as a single string.
        :raises ValueError: If an invalid method is provided.
        """

        if method not in {"delete_build", "make_clean"}:
            raise ValueError(f"Invalid method: {method}. Choose 'delete_build' or 'make_clean'.")

        build_path = os.path.join(det_path, "build")

        method_cmd_map = {
            "delete_build": [
                f"rm -rf {build_path}/*",
                "cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install",
            ],
            "make_clean": [
                f"cd {build_path}",
                "make clean",
            ],
        }

        # assemble recompile commands
        recompile_cmd = [
            f"echo 'Starting build process for detector at {det_path}'",
            f"cd {det_path}",
            *method_cmd_map[method],
            "cmake --build build --target install -- -j$(nproc)",
            f"echo 'Build process completed for {det_path}'",
        ]

        # return commands to recompile
        return recompile_cmd

    def source_det_cmd(self, shell_file_path) -> str:
        source_cmd = [
            f"source '{shell_file_path}'",
            f"echo 'Sourced shell script: {shell_file_path}'"
        ]
        return source_cmd

    def enter_singularity(self, sif_path: str, shell_path: str) -> str:
        eic_shell_path = os.path.join(shell_path, "eic-shell")
        return f"exec singularity {sif_path} {eic_shell_path}"

    def run_cmd(self, curr_cmd: Tuple[str, str, str]) -> None:
        sim_cmd, shell_file_path, det_path = curr_cmd

        # define the commands
        singularity = self.enter_singularity(self.eic_shell_path)
        recompile = self.recompile_det_cmd(det_path, "delete_build")
        source = self.source_det_cmd(shell_file_path)

        commands = [
            "set -e",  # Exit on error
            *singularity,
            *recompile,
            *source,
            f"echo 'Running simulation command: {sim_cmd}'",
            sim_cmd  # Execute simulation command
        ]

        self.logger.info(f"Starting subprocess with command: {sim_cmd}")
        
        try:
            process = subprocess.Popen(
                ["bash", "-c", " && ".join(commands)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            # Stream output line by line
            for line in iter(process.stdout.readline, ''):
                sys.stdout.write(line)  # Print to console
                self.logger.info(line.strip())  # Log to file

            for line in iter(process.stderr.readline, ''):
                sys.stderr.write(line)  # Print errors to console
                self.logger.error(line.strip())  # Log errors to file
            
            process.wait()  # Wait for the process to finish
            if process.returncode != 0:
                self.logger.error(f"Simulation failed with return code {process.returncode}")
                raise RuntimeError(f"Command failed: {sim_cmd}")
            else:
                self.logger.info("Simulation completed successfully.")

        except Exception as e:
            self.logger.error(f"Unexpected error: {e}")
            raise

    def mk_sim_backup(
        self
        ) -> None:
        """
        Method to make a backup of simulation files.
        """
        # create a backup for this run
        os.makedirs(self.backup_path , exist_ok=True)
        self.printlog(f"Created new backup directory in {self.backup_path }")

        # regex pattern to match pixel folders
        px_folder_pattern = re.compile('[0-9]*\.[0-9]*x[0-9]*\.[0-9]*px')

        # move pixel folders to backup
        for item in os.listdir(self.sim_out_path):
            item_path = os.path.join(self.sim_out_path, item)
            # identify folders using regex
            if os.path.isdir(item_path) and px_folder_pattern.match(item):
                shutil.move(item_path, self.backup_path)

        # call function to write the readme file containing the information
        self.setup_readme()

    def setup_readme(
        self
        ) -> None:
        
        # define path for readme file 
        self.readme_path = os.path.join(self.backup_path , "README.txt")
        logging.info(f"Setting up README file at: {self.readme_path}")

        try:
            # get BH value from the function
            self.BH_val = self.get_BH_val()
            logging.info(f"Retrieved BH value: {self.BH_val}")
        
            # get energy levels from file names of genEvents
            self.photon_energy_vals = [
                '.'.join(file.split('_')[1].split('.', 2)[:2]) 
                for file in self.energies 
            ]
            logging.info(f"Extracted photon energy levels: {self.photon_energy_vals}")
        
            #write the README content to the file
            self.printlog(f"Writing simulation information to README file: {self.readme_path}")
            with open(self.readme_path, 'a') as file:
                file.write("SIMULATION INFORMATION:\n")

                file.write(f"py_file: {os.path.basename(__file__)}\n")
                logging.debug(f"Added py_file: {self.file_type}")

                # write each key-value pair from the settings_dict
                for key, value in self.settings_dict.items():
                    file.write(f"{key}: {value}\n")
                    logging.debug(f"Added setting: {key}: {value}")

                file.write(f"file_type: {self.file_type}\n")
                logging.debug(f"Added file_type: {self.file_type}")
                
                file.write(f"Number of Particles: {self.num_particles}\n")
                logging.debug(f"Added num_particles: {self.num_particles}")
                
                file.write(f"Pixel Value Pairs: {self.px_pairs}\n")
                logging.debug(f"Added px_pairs: {self.px_pairs}")
                
                file.write(f"BH: {self.BH_val}\n")
                logging.debug(f"Added BH: {self.BH_val}")
                
                file.write(f"Energy Levels: {self.photon_energy_vals}\n")
                logging.debug(f"Added photon energy levels: {self.photon_energy_vals}")
                
                file.write("\n*********************\n")  
                logging.info("Finished writing to README.")

        except Exception as e:
            logging.error(f"An error occurred while setting up the README: {e}")
            raise 

    def get_BH_val(
        self
        ) -> None:

        # open the path storing the createGenFiles.py file
        with open(self.GenFiles_path, 'r') as file:
            content = file.read()
            
        # use a regex to find the line where BH is defined
        match = re.search(r'BH\s*=\s*(.+)', content)
        
        # if we found BH in the file, we return the value
        if match:
            value = match.group(1).strip()
            return value
        else:
            raise ValueError("Could not find a value for 'BH' in the content of the file.")

if __name__ == "__main__":
    # initialize the simulation handler
    eic_handler = HandleEIC()

    # initialize paths, variables, and settings from JSON
    eic_handler.setup_settings()  
    eic_handler.init_vars()  
    eic_handler.init_paths()  
    os.chmod(os.getcwd(), 0o777)

    # prepare the simulation based on settings
    eic_handler.prep_sim()

    # execute the simulation in parallel
    eic_handler.exec_simv2()

    # make backups after simulations have completed
    eic_handler.mk_sim_backup()


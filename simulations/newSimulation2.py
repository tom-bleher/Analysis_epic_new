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

class HandleEIC(object):

    def __init__(self):
        self.pixel_sizes = []
        self.energy_levels = [] 
        self.default_dx = 1.0 
        self.default_dy = 1.0
        self.sim_dict = {}

    def setup_sim(self) -> None:
        for curr_px_dx, curr_px_dy in self.pixel_sizes:
            # Create respective px folder
            curr_px_path = os.path.join(self.simEvents_path, f"{curr_px_dx}x{curr_px_dy}px")
            os.makedirs(curr_px_path, exist_ok=True)

            # Copy epic folder
            curr_px_epic_path = self.copy_epic(curr_px_path)

            # Rewrite XML to hold current pixel values
            self.rewrite_xml_tree(curr_px_epic_path, curr_px_dx, curr_px_dy)

            # Update the simulation dictionary instead of overwriting
            px_dict = self.create_sim_dict(curr_px_path, curr_px_epic_path, curr_px_dx, curr_px_dy)
            self.sim_dict.update(px_dict)

    def create_sim_dict(self, curr_px_path: str, curr_px_epic_path: str, curr_px_dx: float, curr_px_dy: float) -> dict[str, dict[str, str]]:
        """
        Create simulation dictionary holding 
        "dx_dy"
            "epic"
            "compact"
            "ip6"
            "src_file"
        """
        sim_dict = {}

        px_key = f"{curr_px_dx}_{curr_px_dy}"
        curr_px_path = os.path.join(self.simEvents_path, f"{curr_px_dx}x{curr_px_dy}px")
        os.makedirs(curr_px_path, exist_ok=True)

        curr_px_epic_path = os.path.join(curr_px_path, "epic")

        # Create initial entry for px_key
        sim_dict[px_key] = {
            "px_epic_path": curr_px_epic_path,
            "px_compact_path": curr_px_epic_path + "/install/share/epic/compact",
            "px_ip6_path": curr_px_epic_path + "/install/share/epic/epic_ip6_extended.xml",
            "px_src_path": f"{curr_px_epic_path}/install/bin/",
            "px_out_path": curr_px_path,
        }

        # Now that px_ip6_path exists, populate px_ddsim_cmds
        sim_dict[px_key]["px_ddsim_cmds"] = [self.get_ddsim_cmd(curr_px_path, sim_dict[px_key]["px_ip6_path"], energy)for energy in self.energy_levels]

        return sim_dict

    def init_path_var(self) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """

        self.file_type = "beamEffectsElectrons" # or "idealElectrons" 
        self.num_particles = 5

        self.execution_path = os.getcwd()
        self.det_dir = "/data/tomble/eic/epic"
        self.epicPath = self.det_dir + "/install/share/epic/epic_ip6_extended.xml"

        self.createGenFiles_path = os.path.join(self.execution_path, "createGenFiles.py") # get BH value for generated hepmc files (zero or one)
        self.energy_levels = [file for file in (os.listdir(os.path.join(self.execution_path, "genEvents/results"))) if self.file_type in file]

        self.in_path = ""
        self.out_path = ""
        if len(sys.argv) > 1: 
            self.in_path = f"/{sys.argv[1]}"
        if len(sys.argv) > 2: 
            self.out_path = f"/{sys.argv[2]}"

        if not self.out_path:
            self.out_path = self.in_path

        self.genEvents_path = os.path.join(os.getcwd(), f"genEvents{self.in_path}")
        self.simEvents_path = os.path.join(os.getcwd(), f"simEvents{self.out_path}")

        os.makedirs(self.genEvents_path, exist_ok=True)
        os.makedirs(self.simEvents_path, exist_ok=True)

        # create the path where the simulation file backup will go
        self.backup_path = os.path.join(self.simEvents_path , datetime.now().strftime("%Y%m%d_%H%M%S"))

    def setup_json(self) -> list[tuple]:
        """
        Method for setting up JSON file containing pixel size.
        If the JSON file doesn't exist or is incorrect, a new one is created with default pixel sizes.
        """
        self.px_size_dict = {}
        self.px_json_path = os.path.join(self.execution_path, 'pixel_data.json')
        try:
            # try to read and load the JSON file
            with open(self.px_json_path,'r') as file:
                self.px_size_dict = json.load(file)
            # validate the read data
            px_data = self.px_size_dict['LumiSpecTracker_pixelSize']

            if all(isinstance(item, list) and len(item) == 2 and 
                isinstance(item[0], (float, int)) and isinstance(item[1], (float, int)) for item in px_data):
                self.px_pairs = px_data
            else:
                raise ValueError("Invalid JSON file...")
        except:
            # create a new JSON with default values
            self.px_size_dict = {"LumiSpecTracker_pixelSize": [[self.default_dx, self.default_dy]]}
            with open(self.px_json_path,'w') as file:
                json.dump(self.px_size_dict, file)
            print(f"No valid JSON found. Created JSON file at {self.px_json_path}. Edit the pairs in the JSON to specify your desired values.")
            self.px_pairs = self.px_size_dict['LumiSpecTracker_pixelSize']
        return self.px_pairs

    def copy_epic(self, curr_px_path):
        # copy epic to respective px folder for parameter reference 
        os.system(f'cp -r {self.det_dir} {curr_px_path}')    
        #subprocess.run(["cp", "-r", self.det_dir, curr_px_path], check=True)
        return os.path.join(curr_px_path, "epic")

    def recompile_builds(self) -> None:
        """
        Method to recompile all builds by removing the 'build' directory, recreating it,
        running 'cmake ..', and 'make -j$(nproc)' in each build directory specified by
        the paths in the simulation dictionary.
        """
        for px_key, paths in self.sim_dict.items():
            build_path = os.path.join(paths["px_epic_path"], "build")  # Assuming 'build' is the directory for compilation

            if os.path.exists(build_path):
                print(f"Removing existing build directory: {build_path}")
                subprocess.run(["rm", "-rf", build_path], check=True)

            try:
                # Recreate the build directory
                print(f"Creating new build directory: {build_path}")
                os.makedirs(build_path, exist_ok=True)

                # Run 'cmake ..'
                print(f"Running 'cmake ..' in: {build_path}")
                result = subprocess.run(["cmake", ".."], cwd=build_path, text=True, check=True, capture_output=True)
                print(f"Output of 'cmake ..': {result.stdout}")

                # Run 'make -j$(nproc)'
                print(f"Compiling with 'make -j$(nproc)' in: {build_path}")
                result = subprocess.run(["make", f"-j{os.cpu_count()}"], cwd=build_path, text=True, check=True, capture_output=True)
                print(f"Compilation successful for: {px_key}")
                print(f"Output of 'make -j$(nproc)': {result.stdout}")
            except subprocess.CalledProcessError as e:
                print(f"Error during build process for {px_key} in {build_path}: {e.stderr}")
                raise RuntimeError(f"Compilation failed for {px_key} in {build_path}.")
            except Exception as e:
                print(f"Unexpected error for {px_key} in {build_path}: {str(e)}")
                raise

    def source_shell_script(self, *path_components) -> dict:
        """Sources a shell script and returns the environment variables as a dictionary."""
        script_path = os.path.join(*path_components)

        if not os.path.exists(script_path):
            raise FileNotFoundError(f'Script not found: {script_path}')

        command = ['bash', '-c', 'source "$1" && env', 'bash', script_path]
        try:
            result = subprocess.run(command, stdout=subprocess.PIPE, text=True, check=True)
            env_vars = dict(
                line.split('=', 1) for line in result.stdout.splitlines() if '=' in line
            )
            return env_vars
        except subprocess.CalledProcessError as e:
            raise RuntimeError(f'Failed to source script: {script_path}. Error: {e}')

    def rewrite_xml_tree(self, curr_epic_path, curr_px_dx, curr_px_dy):
        """
        Method for rewriting desired pixel values for all XML files of the Epic 
        detector. For every "{DETECTOR_PATH}" in copied epic XMLs, we replace with the path 
        for the current compact pixel path, and for every compact path 
        we replace with our new compact path 

        Args:
            curr_epic_path
            curr_px_dx
            curr_px_dy
        """

        # iterate over all XML files in the copied epic directory
        for subdir, dirs, files in os.walk(curr_epic_path):
            for filename in files:
                filepath = subdir + os.sep + filename
                if filepath.endswith(".xml") and os.access(filepath, os.W_OK):
                    tree = ET.parse(filepath)
                    root = tree.getroot()
                    for elem in root.iter():
                        if "constant" in elem.tag and 'name' in elem.keys():
                            if elem.attrib['name'] == "LumiSpecTracker_pixelSize_dx":
                                elem.attrib['value'] = f"{curr_px_dx}*mm"
                            elif elem.attrib['name'] == "LumiSpecTracker_pixelSize_dy":
                                elem.attrib['value'] = f"{curr_px_dy}*mm"

                    tree.write(filepath)

    def get_ddsim_cmd(self, curr_px_path, curr_px_epic_ip6_path, energy) -> list:
        """
        Method for setting up the queue of commands, each for executing ddsim.
        """
        inFile = os.path.join(self.genEvents_path, "results", energy)
        match = re.search("\d+\.+\d\.", inFile)
        file_num = match.group() if match else energy.split("_")[1].split(".")[0]
        cmd = f"ddsim --inputFiles {inFile} --outputFile {curr_px_path}/output_{file_num}edm4hep.root --compactFile {curr_px_epic_ip6_path} -N {self.num_particles}"
        return cmd

    def run_cmd(self, cmd_px: tuple) -> None:
        """Executes a command with environment variables sourced from a shell script."""
        cmd, script_dir, script_name = cmd_px
        try:
            env_vars = self.source_shell_script(script_dir, script_name)
            print(f'Executing command: {cmd}')
            result = subprocess.run(
                cmd.split(),
                env=env_vars,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            print(f'Output: {result.stdout}')
            if result.returncode != 0:
                print(f'Error: {result.stderr}')
        except Exception as e:
            print(f'Failed to execute command: {cmd}. Error: {e}')

    def exec_sim(self) -> None:
        """Executes all simulations in parallel using multiprocessing."""

        run_queue = [
            (cmd, self.sim_dict[px_key]['px_src_path'], "thisepic.sh")
            for px_key in self.sim_dict
            for cmd in self.sim_dict[px_key]['px_ddsim_cmds']
        ]

        num_workers = os.cpu_count() 
        with multiprocessing.Pool(num_workers) as pool:
            pool.map(self.run_cmd, run_queue)

    def mk_sim_backup(self) -> None:
        """
        Method to make a backup of simulation files.
        """
        # create a backup for this run
        os.makedirs(self.backup_path , exist_ok=True)
        print(f"Created new backup directory in {self.backup_path }")

        # regex pattern to match pixel folders
        px_folder_pattern = re.compile('[0-9]*\.[0-9]*x[0-9]*\.[0-9]*px')

        # move pixel folders to backup
        for item in os.listdir(self.simEvents_path):
            item_path = os.path.join(self.simEvents_path, item)
            # identify folders using regex
            if os.path.isdir(item_path) and px_folder_pattern.match(item):
                shutil.move(item_path, self.backup_path )

        # call function to write the readme file containing the information
        self.setup_readme()

    def setup_readme(self) -> None:
        
        # define path for readme file 
        self.readme_path = os.path.join(self.backup_path , "README.txt")

        # call the function to read the BH value from the function
        self.BH_val = self.get_BH_val()

        # get energy levels from files names of genEvents
        self.photon_energy_vals = [
            '.'.join(file.split('_')[1].split('.', 2)[:2]) 
            for file in self.energy_levels 
        ]

        # write readme content to the file
        with open(self.readme_path, 'a') as file:
            file.write(f'file_type: {self.file_type}\n')
            file.write(f'Number of Particles: {self.num_particles}\n')
            file.write(f'Pixel Value Pairs: {self.pixel_sizes}\n')
            file.write(f'BH: {self.BH_val}\n')
            file.write(f'Energy Levels : {self.photon_energy_vals}\n')

    def get_BH_val(self):

        # open the path storing the createGenFiles.py file
        with open(self.createGenFiles_path, 'r') as file:
            content = file.read()
            
        # use a regex to find the line where BH is defined
        match = re.search(r'BH\s*=\s*(.+)', content)
        
        # if we found BH in the file, we return the value
        if match:
            value = match.group(1).strip()
            return value
        else:
            raise ValueError("Could not find a value for 'BH' in the content of the file.")

    def get_path(self, *path_components):
        """Joins path components into a single file path."""
        return os.path.join(*path_components)

if __name__ == "__main__":
    # initialize program
    eic_object = HandleEIC()
    eic_object.init_path_var()
    pixel_sizes = eic_object.setup_json()
    eic_object.pixel_sizes = pixel_sizes  
    os.chmod(eic_object.execution_path, 0o777)
    
    # Call setup_sim() to initialize sim_dict
    eic_object.setup_sim()
    print("Simulation dictionary:", eic_object.sim_dict)  # Debugging line

    eic_object.recompile_builds()

    # setup simulation
    eic_object.exec_sim()

    # create backup for simulation
    eic_object.mk_sim_backup()
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
        # we loop over every requested pixel value to 
        # gather the simulation-needed information
        for curr_px_dx, curr_px_dy in self.pixel_sizes:
            
            # create respective px folder which will hold output and more
            curr_px_path = os.path.join(self.simEvents_path, f"{curr_px_dx}x{curr_px_dy}px") 
            os.makedirs(curr_px_path, exist_ok=True) 

            # copy epic and compact folders (os.path.join(curr_px_path, "epic"))
            curr_px_epic_path = self.copy_epic(curr_px_path)

            # rewrite XML to hold current pixel values for all occurrences in epic detector                          
            self.rewrite_xml_tree(curr_px_epic_path, curr_px_dx, curr_px_dy)    

            # gather dictionary per pixel with information needed for run
            self.sim_dict = self.create_sim_dict(curr_px_path, curr_px_epic_path)

    def create_sim_dict(self, curr_px_path, curr_px_epic_path) -> dict:
        """
        Create simulation dictionary holding 
        "dx_dy"
            "epic"
            "compact"
            "ip6"
            "src_file"
        """
        sim_dict = {}

        px_key = f"{curr_dx}_{curr_dy}"
        curr_px_path = os.path.join(self.simEvents_path, f"{curr_dx}x{curr_dy}px")
        os.makedirs(curr_px_path, exist_ok=True) 

        curr_px_epic_path = os.path.join(curr_px_path, "epic")

        sim_dict[px_key] = {
            "px_epic_path": curr_px_epic_path,
            "px_compact_path": curr_px_epic_path + "/install/share/epic/compact",
            "px_ip6_path": curr_px_epic_path + "/install/share/epic/epic_ip6_extended.xml",
            "px_src_path": f"{curr_px_epic_path}/install/bin/thisepic.sh",
            "px_out_path": curr_px_path,
            "px_ddsim_cmds": [self.get_ddsim_cmd(curr_px_path, sim_dict[px_key]["px_ip6_path"], energy) for energy in self.energy_levels]
        }

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
        return os.path.join(curr_px_path, "epic")

    def source_shell_script(self, script_path: str) -> dict:
        """
        Sources a shell script and updates the environment for each subprocess.
        """
        if not os.path.exists(script_path):
            raise FileNotFoundError(f"Script not found: {script_path}")
        
        # Command to source the script and output environment variables
        command = f"bash -c 'source {script_path} && env'"
        
        try:
            # Run the command and capture the output (environment variables)
            result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, text=True, check=True)
            # Parse the environment variables from the command output
            env_vars = dict(
                line.split("=", 1) for line in result.stdout.splitlines() if "=" in line
            )
            return env_vars
        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Failed to source script: {script_path}. Error: {e}")

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

                        # Replace DETECTOR_PATH in element text
                        if elem.text and "${DETECTOR_PATH}" in elem.text:
                            elem.text = elem.text.replace("${DETECTOR_PATH}", f"{curr_epic_path}")
                        # Replace DETECTOR_PATH in attributes
                        for attrib_key, attrib_value in elem.attrib.items():
                            if "${DETECTOR_PATH}" in attrib_value:
                                elem.attrib[attrib_key] = attrib_value.replace("${DETECTOR_PATH}", f"{curr_epic_path}")  
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
        """
        Run a command in the shell after sourcing the environment for a specific pixel configuration.
        
        Args:
            cmd_px (tuple): A tuple containing the command string and the path to the shell script to source.
        """
        cmd, px_src_path = cmd_px
        
        # Source the shell script and get the environment variables
        env_vars = self.source_shell_script(px_src_path)
        
        # Run the command with the sourced environment
        try:
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True, check=True, env=env_vars)
            print(f"Command executed successfully: {cmd}")
            print(f"Output: {result.stdout}")
        except subprocess.CalledProcessError as e:
            print(f"Error executing command: {cmd}")
            print(f"Error output: {e.stderr}")

    def exec_sim(self) -> None:
        """
        Execute all simulations in parallel using multiprocessing.
        """
        # Create a list of (command, px_src_path) tuples for each pixel configuration
        run_queue = [(cmd, self.sim_dict[px_key]["px_src_path"]) 
                    for px_key in self.sim_dict 
                    for cmd in self.sim_dict[px_key]["px_ddsim_cmds"]]
        
        num_workers = os.cpu_count()  # Number of processes
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
    
    # setup simulation
    eic_object.exec_sim()

    # create backup for simulation
    eic_object.mk_sim_backup()
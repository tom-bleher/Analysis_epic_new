# -*- coding: utf-8 -*-
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
import concurrent.futures
import subprocess
import xml.etree.ElementTree as ET

class HandleEIC(object):
    
    def __init__(self) -> None:
        self.setup_var()
        self.setup_paths()
        self.pixelSize_pairs = self.setup_json()

        for px_dx, px_dy in self.pixelSize_pairs:
            self.pixelSize_dx, self.pixelSize_dy = px_dx, px_dy
            self.setup_file_specifics(self.pixelSize_dx, self.pixelSize_dy)

    def setup_var(self) -> None:
        """
        Method for setting variables that control the processing.
        """
        self.file_type = "beamEffectsElectrons" # or "idealElectrons" 
        self.num_particles = 5000
        self.def_px_dx = 0.1 # res in mm
        self.def_px_dy = 0.1 # res in mm

    def setup_paths(self) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """
        self.run_path = os.getcwd()
        self.base_epic_path = f"/data/tomble/eic/epic"
        self.main_xml_path = f"{self.base_epic_path}/install/share/epic/epic_ip6_extended.xml"
        self.createGenFiles_path = f"{self.run_path}/createGenFiles.py" # get BH value for generated hepmc files (zero or one)
        self.energies = [file for file in sorted(os.listdir(f"{self.run_path}/genEvents/results/")) if self.file_type in file]  

        self.in_path = ""
        self.out_path = ""
        if len(sys.argv) > 1: 
            self.in_path = "/" + sys.argv[1]
        if len(sys.argv) > 2: 
            self.out_path = "/" + sys.argv[2]

        if not self.out_path:
            self.out_path = self.in_path

        self.gen_path = f"genEvents{self.in_path}"
        self.sim_path = f"simEvents{self.out_path}"
        self.sim_path_items = os.listdir(self.sim_path)

        # create the path where the simulation file backup will go
        self.SimBackUpPath = os.path.join(self.sim_path, datetime.now().strftime("%Y%m%d_%H%M%S"))

        # if there is no simEvents then create it
        simEvents_path = os.path.join(os.getcwd(), self.sim_path)
        os.makedirs(simEvents_path, exist_ok=True)
        self.set_permission(simEvents_path)

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

    def setup_json(self) -> list[tuple]:
        """
        Method for setting up JSON file containing pixel size.
        If the JSON file doesn't exist or is incorrect, a new one is created with default pixel sizes.
        """
        self.px_dict = {}
        self.px_json_path = os.path.join(self.run_path, 'pixel_data.json')
        try:
            # Try to read and load the JSON file
            with open(self.px_json_path,'r') as file:
                self.px_dict = json.load(file)
            # Validate the read data
            px_data = self.px_dict['LumiSpecTracker_pixelSize']

            if all(isinstance(item, list) and len(item) == 2 and 
                isinstance(item[0], (float, int)) and isinstance(item[1], (float, int)) for item in px_data):
                self.px_pairs = px_data
            else:
                raise ValueError("Invalid JSON file...")
        except:
            # Create a new JSON with default values
            self.px_dict = {"LumiSpecTracker_pixelSize": [[self.def_px_dx, self.def_px_dy]]}
            with open(self.px_json_path,'w') as file:
                json.dump(self.px_dict, file)
            print(f"No valid JSON found. Created JSON file at {self.px_json_path}. Edit the pairs in the JSON to specify your desired values.")
            self.px_pairs = self.px_dict['LumiSpecTracker_pixelSize']
        return self.px_pairs

    def setup_file_specifics(self, curr_px_dx: float, curr_px_dy: float) -> None: 
        """
        Method for setting up file specifics such as creating respective pixel folders, changing definitions xml and 
        looping over all energy levels and saving ddsim commands.
        """
        # create respective px folders and their compact folders
        curr_pix_path = os.path.join(self.sim_path, f"{curr_px_dx}x{curr_px_dy}px") 
        curr_epic_path = os.path.join(curr_pix_path, "epic")
        curr_epic_ip6_path = os.path.join(curr_epic_path, "epic_ip6_extended.xml")

        # create directory for px if it doesn't exist
        os.makedirs(curr_pix_path, exist_ok=True) 
        # os.makedirs(curr_epic_path, exist_ok=True) 

        # set permissions
        self.set_permission(curr_pix_path)
        #self.set_permission(curr_epic_path)

        # copy epic compact to each respective px folder for parameter reference 
        shutil.copytree(self.base_epic_path, curr_pix_path, dirs_exist_ok=True)

        # rewrite {DETECTOR_PATH} and /compact/ for current                                 
        self.rewrite_xml_tree(curr_epic_path, curr_px_dx, curr_px_dy)

        # loop over all energy levels and save ddsim commands
        self.setup_queue(curr_epic_ip6_path, curr_pix_path)

    def rewrite_xml_tree(self, curr_epic_path, curr_px_dx, curr_px_dy):

        # for every "{DETECTOR_PATH}" in copied epic XMLs, we replace with the path for the current compact pixel path 
        # and for every compact path we replace with our new compact path 
        
        # iterate over all XML files in the copied epic directory
        for subdir, dirs, files in os.walk(curr_epic_path):
            for filename in files:
                filepath = subdir + os.sep + filename
                if filepath.endswith(".xml"):
                    tree = ET.parse(filepath)
                    root = tree.getroot()
                    for elem in root.iter():
                        if elem.text:
                            if "{DETECTOR_PATH}" in elem.text:
                                elem.text = elem.text.replace("{DETECTOR_PATH}", f"{curr_epic_path}")
                            elif "<constant name='LumiSpecTracker_pixelSize_dx' value='" in elem.text:
                                elem.text = re.sub(r"<constant name='LumiSpecTracker_pixelSize_dx' value='.*?'>", f"<constant name='LumiSpecTracker_pixelSize_dx' value='{curr_px_dx}*mm'>", elem.text)
                            elif "<constant name='LumiSpecTracker_pixelSize_dy' value='" in elem.text:
                                elem.text = re.sub(r"<constant name='LumiSpecTracker_pixelSize_dy' value='.*?'>", f"<constant name='LumiSpecTracker_pixelSize_dy' value='{curr_px_dy}*mm'>", elem.text)
                        tree.write(filepath)

    def setup_queue(self, curr_epic_ip6_path, curr_pix_path: str) -> dict:
        """
        Method for setting up the queue of commands, each for executing ddsim.
        """
        self.run_queue = {} # init dict to hold commands per file
        for file in self.energies: 
            inFile = self.gen_path + "/results/" + file 
            fileNum = re.search("\d+\.+\d\.", inFile).group() 
            cmd = f"ddsim --inputFiles {inFile} --outputFile {curr_pix_path}/output_{fileNum}edm4hep.root --compactFile {curr_epic_ip6_path} -N {self.num_particles}"
            
            # each file path maps to its associated command
            self.run_queue[inFile] = cmd

        # return dict containing ddsim commands per file
        return self.run_queue

    def exec_sim(self) -> None:
        """
        Method for executing all simulations in parallel using ThreadPoolExecutor.
        """
        # create ThreadPoolExecutor/ProcessPoolExecutor
        with concurrent.futures.ProcessPoolExecutor(max_workers=os.cpu_count()) as executor:
            # submit all tasks at once to the executor
            futures = [executor.submit(self.run_cmd, cmd) for cmd in self.run_queue.values()]

        # Handle completed tasks
        for future in concurrent.futures.as_completed(futures):
            try:
                future.result()  # exceptions are raised here if runSim raised any exception
            except Exception as e:
                print('A command failed with an exception:', str(e))

    def run_cmd(self, cmd) -> None:
        """
        Method to run a command (ddsim execution) using subprocess.
        """
        # start subprocess with command 'cmd'
        process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        # interact with process 
        stdout, stderr = process.communicate()
        # if process fails (nonzero exit code), raise an error
        if process.returncode != 0:        
            raise RuntimeError(f'Failed command {cmd}, Error code {process.returncode}\n{stderr.decode()}')
        
    def setup_readme(self) -> None:
        
        # define path for readme file 
        self.readme_path = os.path.join(self.SimBackUpPath, "README.txt")
        
        # call the function to read the BH value from the function
        self.BH_val = self.get_BH_val()

        # get energy levels from files names of genEvents
        self.photon_energy_vals = [
            '.'.join(file.split('_')[1].split('.', 2)[:2]) 
            for file in self.energies
        ]

        # write readme content to the file
        with open(self.readme_path, 'a') as file:
            file.write(f'file_type: {self.file_type}\n')
            file.write(f'Number of Particles: {self.num_particles}')
            file.write(f'Pixel Value Pairs: {self.pixelSize_pairs}\n')
            file.write(f'BH: {self.BH_val}\n')
            file.write(f'Energies: {self.photon_energy_vals}\n')

    def mk_backup(self) -> None:
        """
        Method to make a backup of simulation files.
        """

        # create a backup for this run
        if len(self.sim_path_items) > 0:
            os.makedirs(self.SimBackUpPath, exist_ok=True)
            self.set_permission(self.SimBackUpPath)
            print(f"Created new backup directory in {self.SimBackUpPath}")

            # move files and pixel folders to backup
            for item in self.sim_path_items:
                item_path = os.path.join(self.sim_path, item)

                if os.path.isfile(item_path) or (os.path.isdir(item_path) and "px" in item):
                    shutil.move(item_path, self.SimBackUpPath)

        # call fucntion to writing the readme file containing the information
        self.setup_readme()


    def set_permission(self, path: str, permission: int = 0o777):
        """
        Method for setting file/directory permissions.

        Args:
            path : path to the file or directory.
            permission : the permissions to be set (in octal), default to 0o777.
        """
        os.chmod(path, permission)

def main():
    # create an object of your class
    eic_object = HandleEIC()

    # using this object, we call the other methods
    eic_object.exec_sim()
    eic_object.mk_backup()

if __name__ == "__main__":
    main()
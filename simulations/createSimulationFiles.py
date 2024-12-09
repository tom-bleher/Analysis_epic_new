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
    
    def __init__(self) -> None:

        self.file_type = "beamEffectsElectrons" # or "idealElectrons" 
        self.num_particles = 5
        self.default_dx = 0.1 # res in mm
        self.default_dy = 0.1 # res in mm

        self.init_path()
        self.pixel_sizes = self.setup_json()

    def main(self) -> None:

        ddsim_queue = []

        # loop over pixel sizes
        for curr_px_dx, curr_px_dy in self.pixel_sizes:
            
            # create respective px folder
            curr_px_path = os.path.join(self.simEvents_path, f"{curr_px_dx}x{curr_px_dy}px") 
            os.makedirs(curr_px_path, exist_ok=True) 

            # copy epic and compact folders (os.path.join(curr_px_path, "epic"))
            curr_px_epic_path = str(self.copy_epic(curr_px_path))

            # rewrite XML to hold current pixel values for all occurrences in epic detector                          
            self.rewrite_xml_tree(curr_px_epic_path, curr_px_dx, curr_px_dy)

            # since we copied the epic folder we can get the relavent ip6 file
            curr_px_epic_ip6 = curr_px_epic_path + "/install/share/epic/epic_ip6_extended.xml"

            # for given pixel, loop over energies
            for energy in self.energy_levels:
                # loop over all energy levels and save ddsim commands
                ddsim_cmd = self.get_ddsim_cmd(curr_px_path, curr_px_epic_ip6, energy)
                ddsim_queue.append(ddsim_cmd)

        # multiprocess the gathered commands 
        self.exec_sim(ddsim_queue)

    def init_path(self) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """
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

        self.genEvents_path  = os.path.join(os.getcwd(), f"genEvents{self.in_path}")
        self.simEvents_path  = os.path.join(os.getcwd(), f"simEvents{self.out_path}")

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

                        # Replace self.det_dir in element text
                        if elem.text and self.det_dir in elem.text:
                            elem.text = elem.text.replace(self.det_dir, f"{curr_epic_path}")
                        # Replace self.det_dir in attributes
                        for attrib_key, attrib_value in elem.attrib.items():
                            if self.det_dir in attrib_value:
                                elem.attrib[attrib_key] = attrib_value.replace(self.det_dir, f"{curr_epic_path}")           

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

    def run_cmd(self, cmd: str) -> None:
        """
        Run a command in the shell
        """
        #subprocess.run(cmd, shell=True, capture_output=True, text=True) TRY POPOPEN HERE
        os.system(cmd)

    def exec_sim(self, run_queue) -> None:
        """
        Execute all simulations in parallel using multiprocessing
        """
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

if __name__ == "__main__":
    eic_object = HandleEIC()
    os.chmod(eic_object.execution_path, 0o777)
    eic_object.main()
    eic_object.mk_sim_backup()

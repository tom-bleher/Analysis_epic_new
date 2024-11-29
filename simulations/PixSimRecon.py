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

    # ---------------------------------------- Part I: EIC ---------------------------------------- #

    def __init__(self) -> None:
        self.source_comp()
        self.init_var()
        self.init_path()
        self.pixel_sizes = self.setup_json()

        # store the coroutines in a list
        self.tasks = [self.prepare_files(dx, dy) for dx, dy in self.pixel_sizes]

    def source_comp(self) -> None:
        """
        Method to source detector and recon
        """
        os.system(f"source {str(os.getcwd()).split('/')[0]}/tomble/Analysis_epic_new/EICrecon/bin/eicrecon-this.sh")
        os.system("source /opt/detector/epic-main/bin/thisepic.sh")

    def init_var(self) -> None:
        """
        Method for setting variables that control the processing.
        """
        self.file_type = "beamEffectsElectrons" # or "idealElectrons" 
        self.num_particles = 1000
        self.default_dx = 0.1 # res in mm
        self.default_dy = 0.1 # res in mm

    def init_path(self) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """
        self.execution_path = os.getcwd()
        os.chmod(self.execution_path, 0o777)
        self.base_epic_path = os.environ['DETECTOR_PATH'] # i.e /data/tomble/eic/epic/install/share/epic
        self.createGenFiles_path = os.path.join(self.execution_path, "createGenFiles.py") # get BH value for generated hepmc files (zero or one)
        self.energy_levels  = [file for file in sorted(os.listdir(os.path.join(self.execution_path, "genEvents/results/"))) if self.file_type in file]  

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
        self.backup_path  = os.path.join(self.simEvents_path , datetime.now().strftime("%Y%m%d_%H%M%S"))

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

    def prepare_files(self, curr_px_dx: float, curr_px_dy: float) -> None: 
        """
        Method for setting up file specifics such as creating respective pixel folders, changing definitions xml and 
        looping over all energy levels and saving ddsim commands.

        Args:
            curr_px_dx
            curr_px_dy

        """
        # create respective px folders and their compact folders
        self.curr_pix_path = os.path.join(self.simEvents_path , f"{curr_px_dx}x{curr_px_dy}px") 
        self.curr_epic_path = os.path.join(self.curr_pix_path, "epic")
        self.curr_epic_ip6_path = os.path.join(self.curr_epic_path, "epic_ip6_extended.xml")

        # create directory for px if it doesn't exist
        os.makedirs(self.curr_pix_path, exist_ok=True) 
        os.makedirs(self.curr_epic_path, exist_ok=True) 

        # copy epic compact to each respective px folder for parameter reference 
        shutil.copytree(self.base_epic_path, self.curr_epic_path, dirs_exist_ok=True)

        # rewrite {DETECTOR_PATH} for current                                 
        self.rewrite_xml_tree(self.curr_epic_path, curr_px_dx, curr_px_dy)

        # loop over all energy levels and save ddsim commands
        self.setup_ddsim_queue()

        # execute those simulations
        self.exec_cmds(self.ddsim_run_queue)

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
                if filepath.endswith(".xml"):
                    tree = ET.parse(filepath)
                    root = tree.getroot()
                    for elem in root.iter():
                        if "constant" in elem.tag and 'name' in elem.keys():
                            if elem.attrib['name'] == "LumiSpecTracker_pixelSize_dx":
                                elem.attrib['value'] = f"{curr_px_dx}*mm"
                            elif elem.attrib['name'] == "LumiSpecTracker_pixelSize_dy":
                                elem.attrib['value'] = f"{curr_px_dy}*mm"
                        elif elem.text:
                            if "{DETECTOR_PATH}" in elem.text:
                                elem.text = elem.text.replace("{DETECTOR_PATH}", f"{curr_epic_path}")
                    tree.write(filepath)

    def setup_ddsim_queue(self) -> dict:
        """
        Method for setting up the queue of commands, each for executing ddsim.
        """
        self.ddsim_run_queue = set() # init set to hold ddsim commands
        for file in self.energy_levels:
            self.inFile = os.path.join(self.genEvents_path,"results/",file)
            match = re.search("\d+\.+\d\.", self.inFile)
            self.file_num = match.group() if match else file.split("_")[1].split(".")[0]
            cmd = f"ddsim --inputFiles {self.inFile} --outputFile {self.curr_pix_path}/output_{self.file_num}edm4hep.root --compactFile {self.curr_epic_ip6_path} -N {self.num_particles}"
            
            # each file path maps to its associated command
            self.ddsim_run_queue.add(cmd)

    def exec_cmds(self, run_queue) -> None:
        """
        Method for executing all simulations in parallel using ThreadPoolExecutor or ProcessPoolExecutor.
        """
        with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:  # or use ProcessPoolExecutor
            future_to_cmd = {executor.submit(self.run_cmd, cmd): cmd for cmd in run_queue}
            
            for future in concurrent.futures.as_completed(future_to_cmd):
                cmd = future_to_cmd[future]
                try:
                    future.result()  # Will raise an exception if run_cmd fails
                except Exception as exc:
                    print(f"Command {cmd} failed with exception: {exc}")
                else:
                    print(f"Command {cmd} completed successfully.")
        
    def run_cmd(self, cmd) -> None:
        """
        Method to run a command (ddsim and recon execution) using subprocess.
        """
        print(f"Running command: {cmd}")  # Debug print
        
        process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()

        # Handle stdout and stderr
        print(f"Standard output: {stdout.decode()}")
        print(f"Standard error: {stderr.decode()}")

        if process.returncode != 0:
            raise RuntimeError(f"Failed command {cmd}, Error code {process.returncode}\n{stderr.decode()}")
            
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

    def mk_backup(self) -> None:
        """
        Method to make a backup of simulation files.
        """

        # create a backup for this run
        os.makedirs(self.backup_path , exist_ok=True)
        print(f"Created new backup directory in {self.backup_path }")

        # regex pattern to match pixel folders
        self.px_folder_pattern = re.compile('[0-9]*\.[0-9]*x[0-9]*\.[0-9]*px')

        # move pixel folders to backup
        for item in os.listdir(self.simEvents_path):
            item_path = os.path.join(self.simEvents_path, item)
            # identify folders using regex
            if os.path.isdir(item_path) and self.px_folder_pattern.match(item):
                shutil.move(item_path, self.backup_path)

        # call function to write the readme file containing the information
        self.setup_readme()

    # ---------------------------------------- Part II: recon ---------------------------------------- #

    def handle_recon(self):
        """
        Method to run recon-related functions
        """

        self.setup_recon_queue()
        self.exec_cmds(self.recon_run_queue)
        self.verify_recon_out()

        # try to multiprocess the failed recons
        if len(self.failed_recon_run_queue) > 0:
            self.exec_cmds(self.failed_recon_run_queue)
        
        # if still fails, run individually
        self.verify_recon_out()
        if len(self.failed_recon_run_queue) > 0:
            for file in self.failed_recon_run_queue:
                if file in cmd:
                    os.system(cmd)

        self.save_comb_recon()

    def setup_recon_queue(self) -> None:
        """
        Method to run EIC recon on created simulation files
        """
        self.recon_run_queue = set() # init set to hold ddsim commands
        # in the backup path, loop over pixel folders
        for px_folder in os.listdir(self.backup_path):
            if os.path.isdir(full_path) and self.px_folder_pattern.match(px_folder):
                file_path = os.path.join(self.backup_path, px_folder)
                for file in os.listdir(file_path):
                    if not self.inFile.endswith(".root"):
                        continue
                    match = re.search("\d+\.+\d\.", self.inFile)
                    self.file_num = match.group() if match else file.split('_')[1][:2]
                    self.inFile = os.path.join(file_path, file)
                    cmd = f"eicrecon -Pplugins=analyzeLumiHits -Phistsfile={f'{file_path}/eicrecon_{fileNum}.root'} {self.in_file}"

                    # each file path maps to its associated command
                    self.recon_run_queue.add(cmd)

    def verify_recon_out(self):
        """
        This methods checks for any failed recon output
        """
        self.failed_recon_run_queue = set()
        for file in sorted(os.listdir(outputPath)):
            filesize = int(os.popen("stat -c %s {0}/{1}".format(outputPath,file)).read())
            if filesize < 1000: # less than 1000 kB is a failed job
                self.failed_recon_run_queue = set()

    def save_comb_recon(self, root_files):
        """
        This method merges all the different roots files
        """
        filesString = ""
        for file in root_files:
            filesString = os.path.join(filesString, " ", "/", file)
        os.system(f"hadd {self.backup_path}/eicrecon_MergedOutput.root {filesString}")

if __name__ == "__main__":
    eic_object = HandleEIC()
    eic_object.mk_backup()
    eic_object.handle_recon()
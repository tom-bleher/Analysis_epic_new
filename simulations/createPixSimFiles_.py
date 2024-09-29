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
from dask.distributed import Client
from dask import compute, delayed

class HandleEIC(object):
    
    def __init__(self) -> None:
        self.setup_var()
        self.setup_paths()
        self.pixelSize_pairs = self.setup_json()
            
        for dx, dy in self.pixelSize_pairs:
            self.pixelSize_dx, self.pixelSize_dy = dx, dy
            self.setup_file_specifics()

    def setup_var(self) -> None:
        """
        Method for setting variables that control the processing.
        """
        self.file_type = "beamEffectsElectrons" # or "idealElectrons" 
        self.tot_proc = os.cpu_count() # set the number of the processors available
        self.num_particles = 1000
        self.def_px_dx = 0.1 # res in mm
        self.def_px_dy = 0.1 # res in mm

    def setup_paths(self) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """
        self.in_path = ""
        self.out_path = ""
        if len(sys.argv) > 1: 
            self.in_path = "/" + sys.argv[1]
        if len(sys.argv) > 2: 
            self.out_path = "/" + sys.argv[2]
        
        # directories genEvents and simEvents needs to exist and manually changed here
        self.epic_path = r"/data/tomble/eic/epic/install/share/epic/epic_ip6_extended.xml"
        self.base_px_xml_path = r"/data/tomble/eic/epic/install/share/epic/compact/far_backward/definitions.xml"
        self.px_json_path = os.getcwd() # json is located in the same path as this python file
        self.gen_path = r"/data/tomble/Analysis_epic_new/simulations/genEvents/results/"
        self.energies = [file for file in sorted(os.listdir(self.gen_path)) if self.file_type in file]  
        self.det_path = os.environ['DETECTOR_PATH']
        self.compact_path = self.det_path + '/compact'

        if not self.out_path:
            self.out_path = self.in_path
       
        self.gen_path = f"genEvents{self.in_path}"
        self.sim_path = f"simEvents{self.out_path}"

        # if there is no simEvents then create it
        simEvents_path = os.path.join(os.getcwd(), self.sim_path)
        os.makedirs(simEvents_path, exist_ok=True)
        os.chmod(simEvents_path, 0o777)

    def setup_json(self) -> list[tuple]:
        """
        Method for setting up JSON file containing pixel size.
        If the JSON file doesn't exist or is incorrect, a new one is created with default pixel sizes.
        """
        self.px_dict = {}
        self.px_json_path = os.path.join(self.px_json_path, 'pixel_data.json')
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

    def setup_file_specifics(self) -> None: 
        """
        Method for setting up file specifics such as creating respective pixel folders, changing definitions xml and 
        looping over all energy levels and saving ddsim commands.
        """
        # loop over requested dx,dy values
        for pair in self.px_pairs: 
            dx, dy = pair
            # create respective px folders and their compact folders
            curr_pix_sim_path = os.path.join(self.sim_path, f"{dx}x{dy}px") 
            curr_compact_path = os.path.join(curr_pix_sim_path, "compact")

            # create directory for px if it doesn't exist
            os.makedirs(curr_pix_sim_path, exist_ok=True) 
            os.makedirs(curr_compact_path, exist_ok=True) 

            # set permissions
            os.chmod(curr_compact_path, 0o777)
            os.chmod(curr_pix_sim_path, 0o777)

            # copy epic compact to each respective px folder for parameter reference 
            shutil.copytree(self.compact_path, curr_compact_path, dirs_exist_ok=True)

            # change definitions xml for each pixel folder 
            self.write_xml(dx, dy, os.path.join(curr_compact_path, 'definitions.xml')) 

            # loop over all energy levels and save ddsim commands
            self.setup_queue(curr_pix_sim_path)

    # for far backwards detectors definition files
    def write_xml(self, px_dx: float, px_dy: float, px_file: str) -> None:
        """
        Method for updating the XML file around pixel values.
        """
        # loop over the copied files and replace the default value with the user input
        with open(px_file, "r+") as file:
            content = file.read()
            # replace the default value with the new value
            content = content.replace(f'<constant name="LumiSpecTracker_pixelSize_dx" value="{self.def_px_dx}*mm"/>',\
                                       f'<constant name="LumiSpecTracker_pixelSize_dx" value="{px_dx}*mm"/>')
            content = content.replace(f'<constant name="LumiSpecTracker_pixelSize_dy" value="{self.def_px_dy}*mm"/>',\
                                       f'<constant name="LumiSpecTracker_pixelSize_dy" value="{px_dy}*mm"/>')
            file.seek(0)
            file.write(content)
            file.truncate()

    def setup_queue(self, curr_pix_sim_path: str) -> dict:
        """
        Method for setting up the queue of commands, each for executing ddsim.
        """
        self.run_queue = {} # init dict to hold commands per file
        for file in self.energies: 
            inFile = self.gen_path + "/results/" + file 
            fileNum = re.search("\d+\.+\d\.", inFile).group() 
            cmd = f"ddsim --inputFiles {inFile} --outputFile {curr_pix_sim_path}/output_{fileNum}edm4hep.root --compactFile {self.epic_path} -N {self.num_particles}"
            
            # each file path maps to its associated command
            self.run_queue[inFile] = cmd

        # return dict containing ddsim commands per file
        return self.run_queue

    def exec_sim(self) -> None:
        """
        Method for executing all simulations in parallel using ThreadPoolExecutor.
        """
        # Create a Dask client
        with Client(processes=True) as client:
            # Transform `self.run_cmd` calls to `delayed` objects
            tasks = [delayed(self.run_cmd)(cmd) for cmd in self.run_queue.values()]
            
            # Compute all delayed objects in parallel
            results = compute(*tasks)

        # Handle Errors
        for result in results:
            if isinstance(result, Exception):
                print('A command failed with an exception:', str(result))

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
        
    def mk_backup(self) -> None:
        """
        Method to make a backup of simulation files.
        """
        # create the path where the simulation file backup will go
        self.SimBackUpPath = os.path.join(self.sim_path, datetime.now().strftime("%Y%m%d_%H%M%S"))

        # create a backup for this run
        if len(os.listdir(self.sim_path)) > 0:
            os.makedirs(self.SimBackUpPath, exist_ok=True)
            os.chmod(self.SimBackUpPath, 0o777)
            print(f"Created new backup directory in {self.SimBackUpPath}")

            # move files and pixel folders to backup
            for item in os.listdir(self.sim_path):
                item_path = os.path.join(self.sim_path, item)

                if os.path.isfile(item_path) or (os.path.isdir(item_path) and "px" in item):
                    shutil.move(item_path, self.SimBackUpPath)

def main():
    # create an object of your class
    eic_object = HandleEIC()

    # using this object, we call the other methods
    eic_object.exec_sim()
    eic_object.mk_backup()

if __name__ == "__main__":
    main()

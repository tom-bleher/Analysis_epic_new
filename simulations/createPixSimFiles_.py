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
from lxml import etree
import asyncio

class HandleEIC(object):
    
    def __init__(self) -> None:
        self.init_var()
        self.init_path()
        self.pixel_sizes = self.setup_json()

        # store the coroutines in a list
        self.tasks = [self.prepare_files() for dx, dy in self.pixel_sizes]

    def init_var(self) -> None:
        """
        Method for setting variables that control the processing.
        """
        self.file_type = "beamEffectsElectrons" # or "idealElectrons" 
        self.num_particles = 10
        self.default_dx = 0.1 # res in mm
        self.default_dy = 0.1 # res in mm

    def init_path(self) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """
        self.execution_path = os.getcwd()
        self.base_epic_path = os.environ['DETECTOR_PATH'] # /data/tomble/eic/epic/install/share/epic
        self.createGenFiles_path = f"{self.execution_path}/createGenFiles.py" # get BH value for generated hepmc files (zero or one)
        self.energy_levels  = [file for file in sorted(os.listdir(f"{self.execution_path}/genEvents/results/")) if self.file_type in file]  

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
        self.set_path_perms(self.simEvents_path)
        self.set_path_perms(self.genEvents_path)

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

    async def prepare_files(self) -> None: 
        """
        Method for setting up file specifics such as creating respective pixel folders, changing definitions xml and 
        looping over all energy levels and saving ddsim commands.
        """
        # create respective px folders and their compact folders
        self.curr_pix_path = os.path.join(self.simEvents_path , f"{self.curr_dx}x{self.curr_dy}px") 
        self.curr_epic_path = os.path.join(self.curr_pix_path, "epic")
        self.curr_epic_ip6_path = os.path.join(self.curr_epic_path, "epic_ip6_extended.xml")

        # create directory for px if it doesn't exist
        os.makedirs(self.curr_pix_path, exist_ok=True) 
        os.makedirs(self.curr_epic_path, exist_ok=True) 

        # set permissions
        self.set_path_perms(self.curr_pix_path)
        self.set_path_perms(self.curr_epic_path)

        # copy epic compact to each respective px folder for parameter reference 
        shutil.copytree(self.base_epic_path, self.curr_epic_path, dirs_exist_ok=True)

        # rewrite {DETECTOR_PATH} for current                                 
        self.rewrite_xml_tree()

        # loop over all energy levels and save ddsim commands
        self.setup_queue()

       # execute those simulations
        await self.exec_sim()

    def rewrite_xml_tree(self) -> None:
        """
        Iterates over all XML files located in the directory specified by curr_epic_path
        to rewrite them in parallel utilizing multiprocessing.
        """

        xml_files = [subdir + os.sep + filename for subdir, dirs, files in os.walk(self.curr_epic_path) for filename in files]
        
        with concurrent.futures.ProcessPoolExecutor(max_workers=os.cpu_count()) as executor:
            # these tasks will be executed in parallel
            executor.map(self.rewrite_one_xml, xml_files, [self.curr_epic_path]*len(xml_files), [self.curr_dx]*len(xml_files), [self.curr_dy]*len(xml_files))

    def rewrite_one_xml(self, filepath, curr_epic_path, curr_px_dx, curr_px_dy) -> None:
        """
        Parses the specified XML file located at filepath and rewrites
        its contents, particularly the DETECTOR_PATH, and pixel dx and dy resolutions.
        """

        if filepath.endswith(".xml"):
            parser = etree.XMLParser(remove_blank_text=True)
            tree = etree.parse(filepath, parser)
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


    def setup_queue(self) -> dict:
        """
        Method for setting up the queue of commands, each for executing ddsim.
        """
        self.run_queue = set() # init set to hold commands
        for file in self.energy_levels : 
            self.inFile = f"{self.genEvents_path}/results/{file}"
            self.file_num = re.search("\d+\.+\d\.", self.inFile).group() 
            cmd = f"ddsim --inputFiles {self.inFile} --outputFile {self.curr_pix_path}/output_{self.file_num}edm4hep.root --compactFile {self.curr_epic_ip6_path} -N {self.num_particles}"
            
            # each file path maps to its associated command
            self.run_queue.add(cmd)

    async def exec_sim(self) -> None:
        """
        Method for executing all simulations in parallel using asyncio.
        """
        # these tasks will be executed in parallel
        await asyncio.gather(*(self.run_cmd(cmd) for cmd in self.run_queue))

    async def run_cmd(self, cmd) -> None:
        """
        Method to run a command (ddsim execution) using asyncio subprocess.
        """
        print(f"Running command: {cmd}")  # debug print

        process = await asyncio.create_subprocess_shell(
            cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        
        stdout, stderr = await process.communicate()
        
        print(f"Standard output: {stdout.decode()}")
        print(f"Standard error: {stderr.decode()}")
        
        if process.returncode != 0: 
            print(f'Failed command {cmd}, Error code {process.returncode}\n{stderr.decode()}')

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
        self.set_path_perms(self.readme_path)

    def mk_sim_backup(self) -> None:
        """
        Method to make a backup of simulation files.
        """

        # create a backup for this run
        os.makedirs(self.backup_path , exist_ok=True)
        self.set_path_perms(self.backup_path)
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

    def set_path_perms(self, path: str, permission: int = 0o777):
        """
        Method for setting file/directory permissions.

        Args:
            path : path to the file or directory.
            permission : the permissions to be set (in octal), default to 0o777.
        """
        os.chmod(path, permission)

if __name__ == "__main__":
    eic_object = HandleEIC()
    loop = asyncio.get_event_loop()

    # now gather the tasks and execute them
    loop.run_until_complete(asyncio.gather(*eic_object.tasks))

    # now you can proceed to executing simulations
    loop.run_until_complete(eic_object.exec_sim())

    eic_object.mk_sim_backup()
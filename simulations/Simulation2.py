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
        self.run_recon = False
        self.file_type = "beamEffectsElectrons" # or "idealElectrons" 
        self.num_particles = 5
        self.default_dx = 0.1 # res in mm
        self.default_dy = 0.1 # res in mm
        self.init_path()
        self.pixel_sizes = self.setup_json()

    def main(self) -> None:
        for (curr_px_dx, curr_px_dy) in self.pixel_sizes:
            print(f"Now running {curr_px_dx}x{curr_px_dy}")

            curr_px_path = os.path.join(self.simEvents_path, f"{curr_px_dx}x{curr_px_dy}px")
            os.makedirs(curr_px_path, exist_ok=True)

            copied_det_path = self.copy_epic(curr_px_path)
            self.rewrite_epicPath(copied_det_path)
            self.rewrite_copied_det(copied_det_path, curr_px_dx, curr_px_dy)

            # Run simulation sequentially for this pixel size
            self.run_simulation(curr_px_path, self.energy_levels)

        self.mk_sim_backup()

    def run_simulation(self, curr_px_path, energy_levels):
        ddsim_queue = [self.get_ddsim_cmd(curr_px_path, energy) for energy in energy_levels]
        print(f"Running simulations for: {curr_px_path}")

        # Parallel execution within this pixel size
        with multiprocessing.Pool(os.cpu_count()) as pool:
            try:
                pool.map(self.run_cmd, ddsim_queue)
            except Exception as e:
                print(f"Error during simulation execution for {curr_px_path}: {e}")
            finally:
                pool.close()
                pool.join()

        print(f"Completed simulations for: {curr_px_path}")

    def init_path(self) -> None:
        """
        Method for setting paths for input, output, and other resources.
        """
        self.execution_path = os.getcwd()
        self.det_dir = "/data/tomble/eic/epic" #/data/tomble/eic/epic/install/share/epic
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

    def rewrite_epicPath(self, copied_det_path):
        # in the sourced detector, go to the ip6 file and change the $DETECTOR_PATH to the copy
        if os.path.isfile(self.epicPath) and os.access(self.epicPath, os.W_OK):
            try:
                # Parse the XML file
                tree = ET.parse(self.epicPath)
                root = tree.getroot()

                # Iterate through all elements in the XML
                for elem in root.iter():
                    # Replace DETECTOR_PATH in element text
                    if elem.text and "${DETECTOR_PATH}" in elem.text:
                        elem.text = elem.text.replace("${DETECTOR_PATH}", f"{copied_det_path}")
                    
                    # Replace DETECTOR_PATH in attributes
                    for attrib_key, attrib_value in elem.attrib.items():
                        if "${DETECTOR_PATH}" in attrib_value:
                            elem.attrib[attrib_key] = attrib_value.replace("${DETECTOR_PATH}", f"{copied_det_path}")

                # Write the modified XML back to the file
                tree.write(self.epicPath)

            except ET.ParseError as e:
                print(f"Failed to parse XML file: {e}")
            except Exception as e:
                print(f"An error occurred: {e}")
        else:
            print("The provided path is not a writable XML file.")

    def rewrite_copied_det(self, copied_det_path, curr_px_dx, curr_px_dy):
        """
        Method for rewriting desired pixel values for all XML files of the Epic 
        detector. For every "{DETECTOR_PATH}" in copied epic XMLs, we replace with the path 
        for the current compact pixel path, and for every compact path 
        we replace with our new compact path 

        Args:
            curr_px_dx
            curr_px_dy
        """

        # iterate over all XML files in the copied epic directory
        for subdir, dirs, files in os.walk(copied_det_path):
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

    def get_ddsim_cmd(self, curr_px_path, energy) -> list:
        """
        Method for setting up the queue of commands, each for executing ddsim.
        """
        inFile = os.path.join(self.genEvents_path, "results", energy)
        match = re.search("\d+\.+\d\.", inFile)
        file_num = match.group() if match else energy.split("_")[1].split(".")[0]         # we changed the $DETECTOR_PATH in the ip6 to out copied detector
        cmd = f"ddsim --inputFiles {inFile} --outputFile {curr_px_path}/output_{file_num}edm4hep.root --compactFile {self.epicPath} -N {self.num_particles}"
        return cmd

    def run_cmd(self, cmd: str) -> None:
        """
        Run a command in the shell and ensure it blocks until completion.
        """
        subprocess.run(cmd, shell=True, check=True)#, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def exec_sim(self, run_queue):
        """Execute all simulations in parallel and block until all are finished."""
        with multiprocessing.Pool(os.cpu_count()) as pool:
            try:
                pool.map(self.run_cmd, run_queue)
            except Exception as e:
                print(f"Error occurred during simulation execution: {e}")
            finally:
                pool.close()
                pool.join()
                
    def mk_sim_backup(self) -> None:
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

    def handle_recon(self):
        """
        Method to run recon-related functions
        """

        self.setup_recon_queue()
        self.exec_sim(self.recon_run_queue)
        self.verify_recon_out()

        # try to multiprocess the failed recons
        if len(self.failed_recon_run_queue) > 0:
            self.exec_sim(self.failed_recon_run_queue)
        
        # if still fails, run individually
        self.verify_recon_out()
        if len(self.failed_recon_run_queue) > 0:
            for file in self.failed_recon_run_queue:
                if file in cmd:
                    os.system(cmd)

        # save the combined root file
        self.save_comb_recon()

    def setup_recon_queue(self) -> None:
        """
        Method to run EIC recon on created simulation files
        """
        self.recon_file_paths = []
        self.recon_run_queue = []

        # in the backup path, loop over pixel folders
        for px_folder in os.listdir(self.backup_path):
            if os.path.isdir(os.path.join(self.backup_path, px_folder)) and self.px_folder_pattern.match(px_folder):
                file_path = os.path.join(self.backup_path, px_folder)
                for file in os.listdir(file_path):
                    if not self.inFile.endswith(".root"):
                        continue
                    match = re.search("\d+\.+\d\.", self.inFile)
                    self.file_num = match.group() if match else file.split('_')[1][:2]
                    self.inFile = os.path.join(file_path, file)
                    cmd = f"eicrecon -Pplugins=analyzeLumiHits -Phistsfile={f'{file_path}/eicrecon_{fileNum}.root'} {self.in_file}"
                    
                    # each file path maps to its associated command
                    self.recon_run_queue.append(cmd)
                    self.recon_file_paths.append(self.inFile)

    def verify_recon_out(self):
        """
        This methods checks for any failed recon output
        """
        self.failed_recon_run_queue = []
        for file in self.recon_file_paths:
            filesize = int(os.popen(f"stat -c %s {self.recon_file_paths}/{file}").read())
            if filesize < 1000: # less than 1000 kB is a failed job
                self.failed_recon_run_queue = []

    def save_comb_recon(self):
        """
        This method merges all the different roots files
        """
        os.system(f"hadd {self.backup_path}/eicrecon_MergedOutput.root {' '.join(self.recon_file_paths)}")

if __name__ == "__main__":
    eic_object = HandleEIC()
    os.chmod(eic_object.execution_path, 0o777)
    eic_object.main()

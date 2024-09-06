# -*- coding: utf-8 -*-
"""
Created on Fri Sep  6 15:33:11 2024

@author: tomble
"""

import os
import shutil
from datetime import datetime
import sys
import re
import multiprocessing

#fileType = "idealElectrons"
fileType = "beamEffectsElectrons"

# directories genEvents and simEvents needs to exist
inPath = ""
outPath = ""
if len(sys.argv) > 1: 
  inPath = "/" + sys.argv[1]
if len(sys.argv) > 2: 
  outPath = "/" + sys.argv[2]

if not outPath:
  outPath = inPath

DEF_PXL_VAL = 0.1
commands = []

genPath = "genEvents{0}".format(inPath)
print(str(genPath))
simPath = "simEvents{0}".format(outPath)
epicPath = "/data/tomble/eic/epic/install/share/epic/epic_ip6_extended.xml"
pixel_def = r"/data/tomble/eic/epic/install/share/epic/compact/far_backward/definitions.xml"

# create the path where the simulation file backup will go
SimBackUpPath = os.path.join(simPath, datetime.now().strftime("%d%m%Y_%H%M%S"))

det_dir = os.environ['DETECTOR_PATH']
compact_dir = det_dir + '/compact'
cmd = 'cp -r {0} {1}'.format(compact_dir, simPath)

# cp over epic compact dir for parameter reference 
os.system('cp -r {0} {1}'.format(compact_dir, simPath) )

# if there is no simEvents then create it
if not os.path.exists(simPath):
    os.mkdir(os.path.join(os.getcwd(),simPath)) 
    print("Out dir doesn't exist.  Created a dir called " + simPath)

# if we have files from a previous run, create a backup and title accordingly 
if any(os.path.isfile(os.path.join(simPath, item)) for item in os.listdir(simPath)):
    os.mkdir(SimBackUpPath)
    for file in os.listdir(simPath):
        shutil.move(os.path.join(simPath, file), SimBackUpPath)
        
# move according compact folder to according folder
if os.path.isdir(os.path.join(simPath, "compact")):
    shutil.move(os.path.join(simPath, "compact"), SimBackUpPath)
    print("Created new back up simulation files in {0}".format(SimBackUpPath))

# prompt user to give pixel values
user_pixel_input = list(input("Enter the desired LumiSpecTracker_pixelSize values seperated by commas. You may press enter to run with the default value."))

def runSims(x):
  os.system(x)

# take user inputs which are currently in str and transform to list with floats
# the default value as above is the base for the xml we are changing if the user
# we add it only for our program's purpose, and we will not run the simulation on unless the user has requested by inserting the default value
pixel_val_list = [float(value) for value in user_pixel_input if "," not in value].insert(0, DEF_PXL_VAL)

# we will run the simulation once for every pixel value configuration in the list
for idx, pixel_value in enumerate(pixel_val_list):

    # loop over the copied files and replace the default value with the user input
    with open(pixel_def, "r+") as file:
        content = file.read()
        # replace the default value with the new value
        content = content.replace(f'<constant name="LumiSpecTracker_pixelSize" value="{pixel_val_list[idx]}*mm"/>', f'<constant name="LumiSpecTracker_pixelSize" value="{pixel_val_list[idx+1]}*mm"/>')
        file.seek(0)
        file.write(content)
        file.truncate()	

    # create command strings
    for file in sorted(os.listdir(r"/data/tomble/Analysis_epic_new/simulations/genEvents/"),):
        #for it in range(1,50):
        if fileType not in file:
          continue
        inFile = genPath + "/results/" + file
        fileNum = re.search("\d+\.+\d\.", inFile).group()
        #fileNum = re.search("\d+\.", inFile).group()
        cmd = "ddsim --inputFiles {0} --outputFile {1}/output_{2}edm4hep.root --compactFile {3} -N 50".format(inFile, simPath, fileNum, epicPath)
        print( cmd )
        commands.append( cmd )

            
    # start Pool of processes
    pool = multiprocessing.Pool(40) # 8 processes to start
    
    # run processes (synchronous, it is a blocking command)
    pool.map( runSims, commands )
    
    # reset commands by emptying the list
    commands.clear()
    
    # make folders according to pixel values (which will be moved to the according date-time folder name in the next run)
    px_val_dir = os.makedir(os.join(simPath, f"{pixel_value}px"))
        
    # move the simulation files generated with the pixel value to an accordingly named directory
    if any(os.path.isfile(os.path.join(simPath, item)) for item in os.listdir(simPath)):
        os.mkdir(SimBackUpPath)
        for file in os.listdir(simPath):
            shutil.move(os.path.join(simPath, file), px_val_dir)
    
    print(f"========================= RAN FOR PIXEL VALUE: {pixel_value} =========================")

    # we just ran on the last index, now reset xml file
    if idx == len(pixel_val_list) - 1:
        # change the original definitions file to the original value once we are done
        with open(pixel_def, "r+") as file:
            content = file.read()
            # replace the default value with the new value
            content = content.replace(f'<constant name="LumiSpecTracker_pixelSize" value="{pixel_val_list[idx]}*mm"/>', f'<constant name="LumiSpecTracker_pixelSize" value="{DEF_PXL_VAL}*mm"/>')
            file.seek(0)
            file.write(content)
            file.truncate()	



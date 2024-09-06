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

genPath = "genEvents{0}".format(inPath)
simPath = "simEvents{0}".format(outPath)
epicPath = "/data/tomble/eic/epic/install/share/epic/epic_ip6_extended.xml"
commands = []

# create the path where the simulation file backup will go
SimBackUpPath = os.path.join(simPath, datetime.now().strftime("%d%m%Y_%H%M%S"))

# if there is no simEvents then create it
if not os.path.exists(simPath):
    os.mkdir(os.path.join(os.getcwd(),simPath)) 
    print("Out dir doesn't exist.  Created a dir called " + simPath)

det_dir = os.environ['DETECTOR_PATH']
compact_dir = det_dir + '/compact'
cmd = 'cp -r {0} {1}'.format(compact_dir, simPath)

# cp over epic compact dir for parameter reference 
os.system('cp -r {0} {1}'.format(compact_dir, simPath) )
        
def runSims(x):
  os.system(x)

# create command strings
for file in sorted(os.listdir(r"/data/tomble/Analysis_epic_new/simulations/genEvents/results/")):
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

if len(os.listdir(simPath)) != 0:
    os.mkdir(SimBackUpPath)
    print("Created new back up directory in {0}".format(SimBackUpPath))
    
    for item in os.listdir(simPath):
        item_path = os.path.join(simPath, item)
        if os.path.isfile(item_path):
            shutil.move(item_path, SimBackUpPath)
    
    # move according compact folder to according folder
    if os.path.isdir(os.path.join(simPath, "compact")):
        shutil.move(os.path.join(simPath, "compact"), SimBackUpPath)
        

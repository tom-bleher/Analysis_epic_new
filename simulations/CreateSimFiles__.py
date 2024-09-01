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
print(f"{genPath}")
simPath = "simEvents{0}".format(outPath)
epicPath = "/data/tomble/eic/epic/install/share/epic/epic_ip6_extended.xml"
SimBackUpPath = os.path.join(simPath, datetime.now().strftime("%d%m%Y_%H%M%S"))

# if there is no simEvents
if not os.path.exists(simPath):
    os.mkdir(os.path.join(os.getcwd(),simPath)) 
    print("Out dir doesn't exist.  Created a dir called " + simPath)

# if we have files from a previous run, create a backup and title accordingly 
if any(os.path.isfile(os.path.join(simPath, item)) for item in os.listdir(simPath)):
    os.mkdir(SimBackUpPath)
    for file in os.listdir(simPath):
        shutil.move(os.path.join(simPath, file), SimBackUpPath)

# move according compact folder
if os.path.isdir(os.path.join(simPath, "compact")):
    print("meow")
    shutil.move(os.path.join(simPath, "compact"), SimBackUpPath)
    print("Created new back up simulation files in {0}".format(SimBackUpPath))

det_dir = os.environ['DETECTOR_PATH']
compact_dir = det_dir + '/compact'
cmd = 'cp -r {0} {1}'.format(compact_dir, simPath)

# cp over epic compact dir for parameter reference 
os.system('cp -r {0} {1}'.format(compact_dir, simPath) )


def runSims(x):
  os.system(x)

commands = []

# create command strings
for file in sorted(os.listdir(r"/data/tomble/Analysis_epic_new/simulations/genEvents/results")):
#for it in range(1,50):
  if fileType not in file:
    continue
  inFile = genPath + "/" + file
  fileNum = re.search("\d+\.+\d\.", inFile).group()
  #fileNum = re.search("\d+\.", inFile).group()
  cmd = "ddsim --inputFiles {0} --outputFile {1}/output_{2}edm4hep.root --compactFile {3} -N 1000".format(inFile, simPath, fileNum, epicPath)
  print( cmd )
  commands.append( cmd )


# start Pool of processes
pool = multiprocessing.Pool(40) # 8 processes to start

# run processes (synchronous, it is a blocking command)
pool.map( runSims, commands )









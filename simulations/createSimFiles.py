import os
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
epicPath = "/home/dhevan/eic/epic/epic_ip6_extended.xml"

if not os.path.exists(simPath):
    print("Out dir doesn't exist.  Create a dir called " + simPath)
    exit()

if len(os.listdir(simPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(simPath))
  exit()

det_dir = os.environ['DETECTOR_PATH']
compact_dir = det_dir + '/compact'
cmd = 'cp -r {0} {1}'.format(compact_dir, simPath)

# cp over epic compact dir for parameter reference 
os.system('cp -r {0} {1}'.format(compact_dir, simPath) )


def runSims(x):
  os.system(x)

commands = []

# create command strings
for file in sorted(os.listdir(genPath),):
#for it in range(1,50):
  if fileType not in file:
    continue
  inFile = genPath + "/" + file
  #fileNum = re.search("\d+\.+\d\.", inFile).group()
  fileNum = re.search("\d+\.", inFile).group()
  cmd = "ddsim --inputFiles {0} --outputFile {1}/output_{2}edm4hep.root --compactFile {3} -N 5000".format(inFile, simPath, fileNum, epicPath)
  print( cmd )
  commands.append( cmd )

# start Pool of processes
pool = multiprocessing.Pool(8) # 8 processes to start

# run processes (synchronous, it is a blocking command)
pool.map( runSims, commands )

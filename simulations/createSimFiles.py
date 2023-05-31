import os
import sys
import re
import multiprocessing

# directories genEvents and simEvents needs to exist
inPath = ""
outPath = ""
if len(sys.argv) > 1: 
  inPath = "/" + sys.argv[1]
if len(sys.argv) > 2: 
  outPath = "/" + sys.argv[2]

if not outPath:
    outPath = inPath

fileType = "beamEffectsElectrons"

genPath = "genEvents{0}".format(inPath)
simPath = "simEvents{0}".format(outPath)
epicPath = "/home/dhevan/eic/epic/epic_ip6_extended.xml"

if len(os.listdir(simPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(simPath))
  exit()

def runSims(x):
  os.system(x)

commands = []

# create command strings
for file in sorted(os.listdir(genPath),):
  if fileType not in file:
    continue
  inFile = genPath + "/" + file
  fileNum = re.search("\d+\.+\d", inFile).group()
  #fileNum = re.search("\d+", inFile).group()
  cmd = "ddsim --inputFiles {0} --outputFile {1}/output_{2}.edm4hep.root --compactFile {3} -N 5000".format(inFile, simPath, fileNum, epicPath)
  print( cmd )
  commands.append( cmd )

# start Pool of processes
pool = multiprocessing.Pool(8) # 8 processes to start

# run processes (synchronous, it is a blocking command)
pool.map( runSims, commands )

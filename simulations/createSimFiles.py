import os
import sys
import re
import multiprocessing

# directories genEvents and simEvents needs to exist
subPath = ""
if len(sys.argv) > 1: 
  subPath = "/" + sys.argv[1]

genPath = "genEvents{0}".format(subPath)
simPath = "simEvents{0}".format(subPath)
epicPath = "/home/dhevan/eic/epic/epic_ip6_extended.xml"

if len(os.listdir(simPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(simPath))
  exit()

def runSims(x):
  os.system(x)

commands = []

# create command strings
for file in sorted(os.listdir(genPath),):
  inFile = genPath + "/" + file
  #fileNum = re.search("\d+\.+\d", inFile).group()
  fileNum = re.search("\d+", inFile).group()
  cmd = "ddsim --inputFiles {0} --outputFile {1}/output_{2}.edm4hep.root --compactFile {3} -N 5000".format(inFile, simPath, fileNum, epicPath)
  commands.append( cmd )

# start Pool of processes
pool = multiprocessing.Pool(8) # 8 processes to start

# run processes
pool.map( runSims, commands ) 


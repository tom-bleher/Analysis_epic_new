import os
import sys
import re

# directories genEvents and simEvents needs to exist
subPath = ""
if len(sys.argv) > 1: 
  subPath = "/" + sys.argv[1]

genPath = "genEvents{0}".format(subPath)
simPath = "simEvents{0}".format(subPath)
epicPath = "/home/dhevan/eic/epic/epic_ip6.xml"

if len(os.listdir(simPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(simPath))
  exit()

for file in sorted(os.listdir(genPath),):
  inFile = genPath + "/" + file
#fileNum = re.search("\d+\.+\d", inFile).group()
  fileNum = re.search("\d+", inFile).group()

#print(fileNum)
# ddsim --inputFiles genEvents/ThreeTrackers/out_18.hepmc --outputFile output.edm4hep.root --compactFile /home/dhevan/eic/epic/epic_ip6.xml -N 5000
  cmd = "ddsim --inputFiles {0} --outputFile {1}/output_{2}.edm4hep.root --compactFile {3} -N 5000".format(inFile, simPath, fileNum, epicPath)
#print(cmd)
  os.system(cmd)


import os
import re

# needs dir genEvents and simEvents to exist
genPath = "genEvents"
simPath = "simEvents"
epicPath = "/home/dhevan/eic/epic/epic_ip6.xml"
count = 0

for file in sorted(os.listdir(genPath),):
  inFile = genPath + "/" + file
  fileNum = int(re.search(r'\d+', inFile).group())
  #print(fileNum)
  cmd = "ddsim --inputFiles {0} --outputFile {1}/output_{2}.edm4hep.root --compactFile {3} -N 1000".format(inFile, simPath, fileNum, epicPath)
  print(cmd)
  #os.system(cmd)
  count += 1


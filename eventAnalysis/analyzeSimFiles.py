import os
import sys
import re
import math
import multiprocessing

# directories genEvents and simEvents needs to exist
simPath = "../simulations/simEvents/ConvertedPhoton_StartConverter"
epicPath = "/home/dhevan/eic/epic/epic_ip6_extended.xml"
outputPath = "output"

# needs dir genEvents to exist
if len(os.listdir(outputPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(outputPath))
  exit()

def runSims(x):
  os.system(x)

commands = []

for file in sorted(os.listdir(simPath),):
  inFile = simPath + "/" + file
  fileNum = re.search("\d+", file).group()
  commands.append("eicrecon -Pplugins=LUMISPECCAL,analyzeLumiHits -PanalyzeLumiHits:Egen={0} -Pjana:nevents=5000 -Ppodio:output_include_collections=EcalLumiSpecIslandProtoClusters -Phistsfile={1}/eicrecon_{0}.root {2}".format(fileNum, outputPath, inFile) )

# start Pool of processes
pool = multiprocessing.Pool(8) # 8 processes to start

# run processes (synchronous, it is a blocking command)
pool.map( runSims, commands )

# merge root files and then delete components
filesString = ""
for file in sorted(os.listdir(outputPath),):
  filesString = filesString + " " + outputPath + "/" + file 

os.system("hadd {0}/MergedOutput.root {1}".format(outputPath, filesString))

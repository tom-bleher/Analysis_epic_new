import os
import sys
import re
import math
from pathlib import Path
import multiprocessing


inDir = ""
outDir = ""
if len(sys.argv) > 1: 
  inDir = sys.argv[1]
if len(sys.argv) > 2: 
  outDir = sys.argv[2]

if not outDir:
  outDir = inDir

if not inDir:
  print("No path provided")
  exit()

# directories genEvents and simEvents needs to exist
simPath = "../simulations/simEvents/" + inDir
outputPath = outDir

if not os.path.exists(outputPath):
    print("Out dir doesn't exist.  Create a dir called " + outputPath)
    exit()

# needs dir genEvents to exist
if len(os.listdir(outputPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(outputPath))
  exit()

def runSims(x):
  os.system(x)

commands = []

for file in sorted(os.listdir(simPath),):
  inFile = simPath + "/" + file
  if ".root" not in inFile:
    continue
  fileNum = re.search("\d+\.+\d\.", inFile).group()
  #fileNum = re.search("\d+\.", file).group()
  #cmd = "eicrecon -Pplugins=LUMISPECCAL,analyzeLumiHits -Ppodio:output_include_collections=EcalLumiSpecClusters,EcalLumiSpecClusterAssociations -Phistsfile={1}/eicrecon_{0}.root {2}".format(fileNum, outputPath, inFile)  
  outFile = outputPath + "/eicrecon_{0}root".format(fileNum)
 
  # Exclude existing files from a previous analysis
  #fileSize = Path(outFile).stat().st_size
  #if Path(outFile).stat().st_size > 1000:
  #continue
  cmd = "eicrecon -Pplugins=analyzeLumiHits -Phistsfile={0} {1}".format(outFile, inFile)  
  print(cmd)
  commands.append( cmd )


# start Pool of processes
pool = multiprocessing.Pool(8) # 8 processes to start

# run processes (synchronous, it is a blocking command)
pool.map( runSims, commands )


# rerun failed jobs sequentially
for file in sorted(os.listdir(outputPath)):
  filesize = int(os.popen("stat -c %s {0}/{1}".format(outputPath,file)).read())
  if filesize < 1000: # less than 1000 kB is a failed job
    for cmd in commands:
      if file in cmd:
        #print( cmd )
        os.system( cmd );

# merge root files and then delete components
filesString = ""
for file in sorted(os.listdir(outputPath),):
  filesString = filesString + " " + outputPath + "/" + file 

os.system("hadd {0}/MergedOutput.root {1}".format(outputPath, filesString))

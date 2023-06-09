import os
import sys
import re
import math
import multiprocessing

inPath = ""

if len(sys.argv) > 1: 
  inPath = sys.argv[1]
if not inPath:
    print("No path provided")
    exit()

# directories genEvents and simEvents needs to exist
simPath = "../simulations/simEvents/" + inPath
outputPath = inPath

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
  fileNum = re.search("\d+\.+\d", inFile).group()
#fileNum = re.search("\d+", file).group()
  cmd = "eicrecon -Pplugins=LUMISPECCAL,analyzeLumiHits -Ppodio:output_include_collections=EcalLumiSpecClusters,EcalLumiSpecClusterAssociations -Phistsfile={1}/eicrecon_{0}.root {2}".format(fileNum, outputPath, inFile)  
  commands.append( cmd )
  print( cmd )

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

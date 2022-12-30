import os
import re
import glob

# needs dir simEvents to exist
simPath = "/home/dhevan/eic/Analysis_epic/eventGen/simEvents"

inFiles = ""

for file in sorted(os.listdir(simPath)):
  inFile = simPath + "/" + file
  E = re.search("\d+\.+\d", inFile).group()
  # run eicrecon
  cmd = "eicrecon -Pplugins=analyzeLumiHits -PanalyzeLumiHits:Egen={0} {1}".format(E, inFile)
  os.system(cmd)  
  #print(cmd)
  # rename output file
  cmd = "mv eicrecon.root eicrecon_{0}.root".format(E)
  os.system(cmd)

# merge all output files together with hadd
ReconFiles = glob.glob("eicrecon_*.root")
ReconFiles = ' '.join(ReconFiles)
cmd = "hadd eicrecon.root {0}".format(ReconFiles)
#print(cmd)
os.system(cmd)
os.system("rm eicrecon_*.root")

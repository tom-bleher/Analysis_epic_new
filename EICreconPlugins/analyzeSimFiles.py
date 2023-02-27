import os
import re
import sys
import glob

# needs dir simEvents to exist
simPath = "/home/dhevan/eic/Analysis_epic/eventGen/simEvents"
Ntrackers = 3 # default

if len(sys.argv) > 1: 
  simPath += "/" + sys.argv[1]
  if sys.argv[1].find("Two") != -1:
      Ntrackers = 2
  if sys.argv[1].find("One") != -1:
      Ntrackers = 1


for file in sorted(os.listdir(simPath)):
  inFile = simPath + "/" + file
#E = re.search("\d+\.+\d", inFile).group()
  E = re.search("\d+", inFile).group()
  # run eicrecon
# eicrecon -Pplugins=LUMISPECCAL,analyzeLumiHits -PanalyzeLumiHits:Egen=18 -PanalyzeLumiHits:Ntrackers=3  ../eventGen/test.edm4hep.root
  cmd = "eicrecon -Pplugins=LUMISPECCAL,analyzeLumiHits -PanalyzeLumiHits:Egen={0} -PanalyzeLumiHits:Ntrackers={1} -Phistfile=eicrecon.root {2}".format(E, Ntrackers, inFile)
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

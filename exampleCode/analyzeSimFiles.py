import os
import re
import sys
import glob

# needs dir simEvents to exist
simPath = "/Users/simbawobogo/eic/simEvents"

for file in sorted(os.listdir(simPath)):
  inFile = simPath + "/" + file
#E = re.search("\d+\.+\d", inFile).group()
  E = re.search("\d+", inFile).group()
  # run eicrecon
  cmd = "eicrecon -Pplugins=SimpleAcceptance -PSimpleAcceptance:Egen={0} -Phistsfile=eicrecon_{0}.root {1}".format(E, inFile)
  print(cmd)
  os.system(cmd)  

# merge all output files together with hadd
ReconFiles = glob.glob("eicrecon_*.root")
ReconFiles = ' '.join(ReconFiles)
cmd = "hadd eicrecon.root {0}".format(ReconFiles)
#print(cmd)
os.system(cmd)
os.system("rm eicrecon_*.root")

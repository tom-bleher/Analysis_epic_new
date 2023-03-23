import os
import sys
import re

for E in range(5,35):

  cmd = "ddsim --inputFiles genEvents/out_{0}.hepmc --outputFile simEvents/output_{0}.edm4hep.root --compactFile epic/epic_ip6.xml -N 500".format(E) 
  print(cmd)
  os.system(cmd)


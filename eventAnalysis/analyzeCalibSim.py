import os
import sys
import re
import math

startModule = 1
endModule = 1

if len(sys.argv) == 3: 
  startModule = int(sys.argv[1])
  endModule = int(sys.argv[2])

for mod in range(startModule, endModule + 1):

#cmd = "eicrecon -Pplugins=LUMISPECCAL,analyzeLumiHits -PanalyzeLumiHits:Egen=10 -PanalyzeLumiHits:Ntrackers=0 -Phistsfile=calibrationsPbWO4/eicrecon_{0}.root ../simulations/simEvents/calibrationPbWO4/output_{0}.edm4hep.root".format(mod)
  cmd = "eicrecon -Pplugins=LUMISPECCAL,analyzeLumiHits -PanalyzeLumiHits:Egen=10 -PanalyzeLumiHits:Ntrackers=0 ../simulations/simEvents/calibrationPbWO4/output_{0}.edm4hep.root".format(mod)

  print(cmd)
  os.system(cmd)
  cmd = "mv eicrecon.root calibrationsPbWO4/eicrecon_{0}.root".format(mod)
  print(cmd)
  os.system(cmd)

import os
import sys

# POS.ConvStart, POS.ConvEnd, POS.SweeperEnd
location = "POS.SweeperEnd"

genPath = "genEvents"

if len(sys.argv) > 1: 
  genPath = genPath + "/" + sys.argv[1]

# needs dir genEvents to exist
if len(os.listdir(genPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(genPath))
  exit()

for n in range(30):
  E = 5 + n/2
  #cmd = "root -q 'lumi_particles.cxx(1e4,true,true,true,{0},{0},\"{1}/out_{0}.hepmc\")'".format(E,genPath)
  cmd = "root -q 'lumi_particles.cxx(1e4,true,false,false,{0},{0},\"{1}/PhotonsAtIP_{0}.hepmc\")'".format(E,genPath)
  os.system(cmd)

  # beam effects
  cmd = "abconv {1}/PhotonsAtIP_{0}.hepmc --plot-off -o {1}/beamEffectsPhotonsAtIP_{0}".format(E,genPath)
  os.system(cmd)

  # propagate to certain locations, specified as last argument
  cmd = "root -b 'PropagateAndConvert.cxx(\"{1}/beamEffectsPhotonsAtIP_{0}.hepmc\",\"{1}/beamEffectsElectrons_{0}.hepmc\",{2})'".format(E,genPath,location)
  os.system(cmd)
  cmd = "root -b 'PropagateAndConvert.cxx(\"{1}/PhotonsAtIP_{0}.hepmc\",\"{1}/Electrons_{0}.hepmc\",{2})'".format(E,genPath,location)
  os.system(cmd)


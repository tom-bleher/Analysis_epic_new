import os
import sys

# BH (1) or flat E spectrum (0)
BH = 0

# POS.ConvMiddle POS.AnalyzerStart
location = "POS.ConvMiddle"
#location = "POS.AnalyzerStart"

genPath = "genEvents"

energies = [10,15,20,25,30] # GeV

if len(sys.argv) > 1: 
  genPath = genPath + "/" + sys.argv[1]
else:
  print("specify dir to store events")
  exit()

# needs dir genEvents to exist
if len(os.listdir(genPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(genPath))
  exit()

for n in energies:
  ID = n # just the index of statistically independent sample
  if BH == 1: # BH E spectrum
    cmd = "root -q 'lumi_particles.cxx(1e4,false,false,false,4,18,\"{1}/idealPhotonsAtIP_{0}.hepmc\")'".format(ID,genPath)
  else: # flat E spectrum
    cmd = "root -q 'lumi_particles.cxx(1e4,true,false,false,{0},{0},\"{1}/idealPhotonsAtIP_{0}.hepmc\")'".format(ID,genPath)

  os.system(cmd)

  # beam effects
  cmd = "abconv {1}/idealPhotonsAtIP_{0}.hepmc --plot-off -o {1}/beamEffectsPhotonsAtIP_{0}".format(ID,genPath)
  os.system(cmd)

  # propagate to certain locations, specified as last argument
  cmd = "root -b 'PropagateAndConvert.cxx(\"{1}/beamEffectsPhotonsAtIP_{0}.hepmc\",\"{1}/beamEffectsElectrons_{0}.hepmc\",{2})'".format(ID,genPath,location)
  os.system(cmd)
  cmd = "root -b 'PropagateAndConvert.cxx(\"{1}/idealPhotonsAtIP_{0}.hepmc\",\"{1}/idealElectrons_{0}.hepmc\",{2})'".format(ID,genPath,location)
  os.system(cmd)

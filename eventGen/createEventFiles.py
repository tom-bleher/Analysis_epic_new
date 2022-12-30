import os

genPath = "genEvents/beamEffects"

# needs dir genEvents to exist
if len(os.listdir(genPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(genPath))
  exit()

for n in range(30):
  E = 5 + n
  cmd = "root -q 'lumi_particles.cxx(1e5,true,true,{0},{0},\"{1}/out_{0}.hepmc\")'".format(E,genPath)
  #print(cmd)
  os.system(cmd)


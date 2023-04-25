import os

genPath = "genEvents/ConvertedPhoton_StartConverter"

# needs dir genEvents to exist
if len(os.listdir(genPath)) != 0:
  print("{0} directory not empty.  Clear directory".format(genPath))
  exit()

for n in range(35):
  E = 1 + n
  cmd = "root -q 'lumi_particles.cxx(1e4,true,true,{0},{0},\"{1}/out_{0}.hepmc\")'".format(E,genPath)
  #print(cmd)
  os.system(cmd)


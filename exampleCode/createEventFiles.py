import os

for E in range(5,35):
  cmd = "root -q 'lumi_particles.cxx(1e4,true,true,{0},{0},\"genEvents/out_{0}.hepmc\")'".format(E)
  print(cmd)
  os.system(cmd)


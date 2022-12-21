import os

# needs dir genEvents to exist
for n in range(30):
  E = 5 + n/2.
  cmd = "root -q 'lumi_particles.cxx(1e5,true,true,{0},{0},\"genEvents/out_{1}.hepmc\")'".format(E,n)
  #print(cmd)
  os.system(cmd)


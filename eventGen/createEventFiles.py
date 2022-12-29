import os

# needs dir genEvents to exist
if len(os.listdir("genEvents")) != 0:
  print("genEvents directory not empty.  Clear directory")
  exit()

for n in range(60):
  E = 5 + n/2.
  cmd = "root -q 'lumi_particles.cxx(1e5,true,true,{0},{0},\"genEvents/out_{0}.hepmc\")'".format(E)
  #print(cmd)
  os.system(cmd)


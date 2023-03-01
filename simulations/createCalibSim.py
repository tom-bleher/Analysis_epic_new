import os
import sys
import re
import math

# directories genEvents and simEvents needs to exist
startModule = 1
endModule = 1
if len(sys.argv) == 3: 
  startModule = int(sys.argv[1])
  endModule = int(sys.argv[2])

epicPath = "/home/dhevan/eic/epic/epic_ip6.xml"

# sizes in mm
moduleSize = 20
middleGap = 2*69
MagnetEndToCALface = 8120

NmodulesX = 10
NmodulesY = 10
mod = 0

for modX in range(NmodulesX):
  for modY in range(NmodulesY):
    mod = mod + 1

    if mod < startModule:
        continue
    if mod > endModule:
        continue

    X = -moduleSize*NmodulesX/2 + (modX + 1/2)*moduleSize
    Y = middleGap/2 + (modY + 1/2)*moduleSize
    Z = -MagnetEndToCALface
    R = math.sqrt(X*X + Y*Y + Z*Z)
    gunX = X / R
    gunY = Y / R
    gunZ = Z / R
    
    print(str(X) + '   ' + str(Y))
    print(str(gunX) + '   ' + str(gunY) + '   ' + str(gunZ))
    
    cmd = "ddsim --steeringFile steeringGun.py --gun.direction '({0}, {1}, {2})' --outputFile simEvents/calibrationPbWO4/output_{3}.edm4hep.root --compactFile {4} -N 1000".format(gunX, gunY, gunZ, mod, epicPath)
    print(cmd)
    os.system(cmd)

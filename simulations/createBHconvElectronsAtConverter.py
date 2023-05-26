import os

os.system("root -b 'lumi_particles.cxx(1e3, false, false, false, 5,18, \"genParticles.hepmc\")'")
os.system("abconv genParticles.hepmc --plot-off -o beamEffects")
os.system("root -b 'PropagateAndConvert.cxx(\"beamEffects.hepmc\",\"converterElectrons.hepmc\",POS.ConvStart)'")


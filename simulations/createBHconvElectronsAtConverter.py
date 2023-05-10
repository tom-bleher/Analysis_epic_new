import os

os.system("root -b 'lumi_particles.cxx(1e5, false, false, false, 5,18, \"genParticles.hepmc\")'")
os.system("abconv genParticles.hepmc --plot-off -o beamEffects")
os.system("root -b 'PropToConverterAndConvert.cxx(\"beamEffects.hepmc\",\"converterElectrons.hepmc\")'")


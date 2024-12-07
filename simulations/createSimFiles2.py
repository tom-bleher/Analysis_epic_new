import os
import sys
import re
import multiprocessing
import shutil 
from datetime import datetime  

def runSims(x):
  os.system(x)

def run() -> None:
    # run processes (synchronous, it is a blocking command)
    pool = multiprocessing.Pool(os.cpu_count())
    pool.map(runSims, commands)

def paths() -> None:
    in_path = ""
    out_path = ""
    if len(sys.argv) > 1: 
        in_path = f"/{sys.argv[1]}"
    if len(sys.argv) > 2: 
        out_path = f"/{sys.argv[2]}"

    if not out_path:
        out_path = in_path

    genEvents_path  = os.path.join(os.getcwd(), f"genEvents{in_path}")
    simEvents_path  = os.path.join(os.getcwd(), f"simEvents{out_path}")

    os.makedirs(genEvents_path, exist_ok=True)
    os.makedirs(simEvents_path, exist_ok=True)

    # create the path where the simulation file backup will go
    backup_path = os.path.join(simEvents_path , datetime.now().strftime("%Y%m%d_%H%M%S"))

    return genEvents_path, simEvents_path

def gather_cmds(file_type, genPath, simPath, epicPath, num_particles) -> None:
    commands = []
    # create command strings
    for file in sorted(os.listdir(gen_path)):
        if file_type not in file:
            continue
        inFile = genPath + "/results/" + file
        match = re.search("\d+\.+\d\.", inFile)
        fileNum = match.group() if match else file.split("_")[1].split(".")[0]
        cmd = "ddsim --inputFiles {0} --outputFile {1}/output_{2}edm4hep.root --compactFile {3} -N {4}".format(inFile, simPath, fileNum, epicPath, num_particles)
        print(cmd)
        commands.append(cmd)
    return commands

file_type = "beamEffectsElectrons"
det_dir = "/data/tomble/eic/epic" 
epicPath = det_dir + "/install/share/epic/epic_ip6_extended.xml"
gen_path = "/data/tomble/Analysis_epic_new/simulations/genEvents/results"
num_particles = 1000

gen_path, sim_path = paths() # setup paths
commands = gather_cmds(file_type, gen_path, sim_path, epicPath, num_particles) # gather commands
run() # run simulation

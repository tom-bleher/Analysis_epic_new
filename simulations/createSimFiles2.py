import os
import sys
import re
import multiprocessing

paths() # setup paths
commands = gather_cmds() # gather commands
run() # run simulation
mk_sim_backup() # make backup

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

def gather_cmds() -> None:
    commands = []
    # create command strings
    for file in sorted(os.listdir(r"/data/tomble/Analysis_epic_new/simulations/genEvents/results"),):
        if fileType not in file:
            continue
        inFile = genPath + "/results/" + file
        match = re.search("\d+\.+\d\.", inFile)
        fileNum = match.group() if match else file.split("_")[1].split(".")[0]
        cmd = "ddsim --inputFiles {0} --outputFile {1}/output_{2}edm4hep.root --compactFile {3} -N 1000".format(inFile, simPath, fileNum, epicPath)
        print(cmd)
        commands.append(cmd)
    return commands

def mk_sim_backup() -> None:
    """
    Method to make a backup of simulation files.
    """
    # create a backup for this run
    os.makedirs(backup_path , exist_ok=True)
    print(f"Created new backup directory in {backup_path }")

    # regex pattern to match pixel folders
    px_folder_pattern = re.compile('[0-9]*\.[0-9]*x[0-9]*\.[0-9]*px')

    # move pixel folders to backup
    for item in os.listdir(simEvents_path):
        item_path = os.path.join(simEvents_path, item)
        # identify folders using regex
        if os.path.isdir(item_path) and px_folder_pattern.match(item):
            shutil.move(item_path, backup_path)

    # call function to write the readme file containing the information
    #setup_readme()

def setup_readme(backup_path, ) -> None:
    
    # define path for readme file 
    readme_path = os.path.join(backup_path, "README.txt")

    # call the function to read the BH value from the function
    BH_val = get_BH_val()

    # get energy levels from files names of genEvents
    photon_energy_vals = [
        '.'.join(file.split('_')[1].split('.', 2)[:2]) 
        for file in energy_levels 
    ]

    # write readme content to the file
    with open(readme_path, 'a') as file:
        file.write(f'file_type: {file_type}\n')
        file.write(f'Number of Particles: {num_particles}\n')
        file.write(f'Pixel Value Pairs: {pixel_sizes}\n')
        file.write(f'BH: {BH_val}\n')
        file.write(f'Energy Levels : {photon_energy_vals}\n')

def get_BH_val(createGenFiles_path):

    # open the path storing the createGenFiles.py file
    with open(createGenFiles_path, 'r') as file:
        content = file.read()
        
    # use a regex to find the line where BH is defined
    match = re.search(r'BH\s*=\s*(.+)', content)
    
    # if we found BH in the file, we return the value
    if match:
        value = match.group(1).strip()
        return value
    else:
        raise ValueError("Could not find a value for 'BH' in the content of the file.")


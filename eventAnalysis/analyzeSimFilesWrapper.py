import subprocess
import time
import os 

# Modify the file here by searching for the string and replacing it
def modify_path(file_path, old_path, new_path):
    with open(file_path, 'r') as file:
        content = file.read()

    # Replace the old path with the new one
    content = content.replace(old_path, new_path)
    with open(file_path, 'w') as file:
        file.write(content)
    
    
    os.makedir(os.path.join(os.getcwd(),os.path.join(*new_path.split('/')[-2:])))

# Run the Python file
def run_python_file(file_path):
    result = subprocess.run(['python', file_path], capture_output=True, text=True)
    print(f"Output of the script:\n{result.stdout}")
    if result.stderr:
        print(f"Errors:\n{result.stderr}")

def main(path_name, prev_path):
    file_path = 'analyzeSimFiles.py' 
    run_python_file(file_path)  # Run the file with the previous path
    modify_path(file_path, prev_path, path_name)  # Modify the file to use the new path
    time.sleep(3)  # Pause before running the modified file
    run_python_file(file_path)  # Run the modified file

# List of simulation outputs
sim_out_paths = ["/data/tomble/Analysis_epic_new/simulations/simEvents/20241126_000552/0.1x0.1px","/data/tomble/Analysis_epic_new/simulations/simEvents/20241126_000552/2.0x0.1px","/data/tomble/Analysis_epic_new/simulations/simEvents/20241126_015234/0.1x0.1px","/data/tomble/Analysis_epic_new/simulations/simEvents/20241126_015234/2.0x0.1px"]

# Loop through the pathtion versions
for i in range(1, len(sim_out_paths)):
    main(sim_out_paths[i], sim_out_paths[i-1])  # Pass the current version and the previous version

if __name__ == "__main__":
    main("/data/tomble/Analysis_epic_new/simulations/simEvents/20241126_000552/0.1x0.1px", "")

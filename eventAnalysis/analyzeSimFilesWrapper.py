import subprocess
import time
import os 

def modify_path(file_path, old_path, new_path):
    with open(file_path, 'r') as file:
        content = file.read()

    # Replace the old path with the new one
    content = content.replace(old_path, new_path)
    with open(file_path, 'w') as file:
        file.write(content)

    # Create the corresponding output directory
    output_folder = os.path.join(os.getcwd(), os.path.join(*new_path.split('/')[-2:]).replace("/", "_")).replace(".", "_")
    print(f"===================================={output_folder}")
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
    return output_folder

def run_python_file(file_path, *args):
    # Run the Python file with additional arguments
    result = subprocess.run(['python', file_path, *args], capture_output=True, text=True)
    print(f"Output of the script:\n{result.stdout}")
    if result.stderr:
        print(f"Errors:\n{result.stderr}")

def main(path_name, prev_path):
    file_path = 'analyzeSimFiles.py'
    output_dir = modify_path(file_path, prev_path, path_name)
    time.sleep(1)  # Pause before running the modified file
    run_python_file(file_path, path_name, output_dir)  # Pass the new path and output dir as arguments

# List of simulation outputs
sim_out_paths = [
    "/data/tomble/Analysis_epic_new/simulations/simEvents/20241126_000552/0.1x0.1px",
    "/data/tomble/Analysis_epic_new/simulations/simEvents/20241126_000552/2.0x0.1px",
    "/data/tomble/Analysis_epic_new/simulations/simEvents/20241126_015234/0.1x0.1px",
    "/data/tomble/Analysis_epic_new/simulations/simEvents/20241126_015234/2.0x0.1px"
]

if __name__ == "__main__":
    for i in range(len(sim_out_paths)):
        prev_path = sim_out_paths[i - 1] if i > 0 else ""
        main(sim_out_paths[i], prev_path)
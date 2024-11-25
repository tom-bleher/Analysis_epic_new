import subprocess
import time

# Modify the file here by searching for the string and replacing it
def modify_func(file_path, old_func, new_func):
    with open(file_path, 'r') as file:
        content = file.read()

    # Replace the old function call (v1) with the new one (v2, v3, etc.)
    content = content.replace(old_func, new_func)

    with open(file_path, 'w') as file:
        file.write(content)

# Run the Python file
def run_python_file(file_path):
    result = subprocess.run(['python', file_path], capture_output=True, text=True)
    print(f"Output of the script:\n{result.stdout}")
    if result.stderr:
        print(f"Errors:\n{result.stderr}")

def main(func_name, prev_func):
    file_path = 'createPixSimFiles_test.py' 
    run_python_file(file_path)  # Run the file with the previous function
    modify_func(file_path, prev_func, func_name)  # Modify the file to use the new function
    time.sleep(5)  # Pause before running the modified file
    run_python_file(file_path)  # Run the modified file

# List of function names to use (v1, v2, v3, v4)
func_versions = ["exec_simv1", "exec_simv2", "exec_simv3", "exec_simv4"]

# Loop through the function versions
for i in range(1, len(func_versions)):
    main(func_versions[i], func_versions[i-1])  # Pass the current version and the previous version

if __name__ == "__main__":
    main("exec_simv1", "")  # Start with v1 (no previous function)

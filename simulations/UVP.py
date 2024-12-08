import os
import subprocess

class HandleEIC:

    def __init__(self):
        self.execution_path = os.getcwd()
        self.simEvents_path = os.path.join(self.execution_path, "simEvents")
        self.backup_path = os.path.join(self.simEvents_path, "backup")
        self.createGenFiles_path = os.path.join(self.execution_path, "createGenFiles.py")

    def source_shell_script(self, script_path: str) -> dict:
        """
        Sources a shell script and updates the environment for each subprocess.
        """
        print(f"Attempting to source shell script at: {script_path}")
        if not os.path.exists(script_path):
            print(f"ERROR: Script not found: {script_path}")
            return {}

        # Command to source the script and output environment variables
        command = f"bash -c 'source {script_path} && env'"
        print(f"Running command: {command}")
        
        try:
            # Run the command and capture the output (environment variables)
            result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
            print(f"Command executed successfully. Output:")
            print(result.stdout)
            # Parse the environment variables from the command output
            env_vars = dict(
                line.split("=", 1) for line in result.stdout.splitlines() if "=" in line
            )
            print(f"Extracted environment variables: {env_vars}")
            return env_vars
        except subprocess.CalledProcessError as e:
            print(f"ERROR: Failed to source script: {script_path}. Error: {e}")
            print(f"stderr: {e.stderr}")
            return {}

    def run_cmd(self, cmd_px: tuple) -> None:
        """
        Run a command in a subprocess after sourcing the environment from the shell script.
        """
        cmd, px_src_path = cmd_px
        print(f"Preparing to run command: {cmd} with source path: {px_src_path}")
        
        try:
            # Get environment variables by sourcing the shell script
            print(f"Calling source_shell_script for: {px_src_path}")
            env_vars = self.source_shell_script(px_src_path)
            if not env_vars:
                print(f"ERROR: No environment variables retrieved. Skipping command execution.")
                return
            
            print(f"Executing command: {cmd}")
            # Run the command with the sourced environment
            result = subprocess.run(cmd, shell=True, env={**os.environ, **env_vars}, 
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            print(f"Output: {result.stdout}")
            if result.returncode != 0:
                print(f"Error: {result.stderr}")
        except Exception as e:
            print(f"Failed to execute command: {cmd}. Error: {e}")


# Example of usage
if __name__ == "__main__":
    print("Starting the HandleEIC example...")
    
    # Create a HandleEIC object
    eic_object = HandleEIC()
    
    # Simulate a pixel source path (you should change this to a valid path in your environment)
    simulated_script_path = os.path.join(eic_object.execution_path, "example_script.sh")
    
    # This is a minimal command to run after sourcing the environment
    cmd = "echo 'Hello from the subprocess!'"
    
    # Run a simulated command using the sourced environment
    eic_object.run_cmd((cmd, simulated_script_path))
    
    print("Example finished.")

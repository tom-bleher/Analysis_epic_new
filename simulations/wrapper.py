import json
import subprocess
import time
from datetime import datetime
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np 

def update_json_settings(particle_count):
    """Update the simulation settings JSON file with new particle count."""
    settings_file = "simulation_settings.json"
    
    try:
        with open(settings_file, 'r') as f:
            settings = json.load(f)
            
        # Update the particle count
        settings['num_particles'] = [particle_count]  # Keeping it as a list for compatibility
        
        with open(settings_file, 'w') as f:
            json.dump(settings, f, indent=2)
            
        return True
    except Exception as e:
        print(f"Error updating settings file: {e}")
        return False

def run_simulation():
    """Run the epic simulation script and capture its output."""
    try:
        start_time = time.time()
        result = subprocess.run(
            ["python3", "epic_sim.py"],
            capture_output=True,
            text=True,
            check=True
        )
        end_time = time.time()
        
        return {
            'success': True,
            'output': result.stdout,
            'error': result.stderr,
            'duration': end_time - start_time
        }
    except subprocess.CalledProcessError as e:
        return {
            'success': False,
            'output': e.stdout,
            'error': e.stderr,
            'duration': time.time() - start_time
        }

def create_performance_report(results):
    """Create a performance report and plots from the simulation results."""
    # Create results directory
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    results_dir = f"performance_results_{timestamp}"
    os.makedirs(results_dir, exist_ok=True)
    
    # Prepare data for DataFrame
    data = {
        'particles': [],
        'duration': [],
        'success': []
    }
    
    for particle_count, result in results.items():
        data['particles'].append(particle_count)
        data['duration'].append(result['duration'])
        data['success'].append(result['success'])
    
    # Create DataFrame and save to CSV
    df = pd.DataFrame(data)
    csv_path = os.path.join(results_dir, 'performance_results.csv')
    df.to_csv(csv_path, index=False)
    
    # Create performance plot
    plt.figure(figsize=(10, 6))
    plt.plot(df['particles'], df['duration'], 'b-o')
    plt.xscale('log')
    plt.xlabel('Number of Particles')
    plt.ylabel('Duration (seconds)')
    plt.title('Simulation Performance vs Particle Count')
    plt.grid(True)
    
    # Save plot
    plot_path = os.path.join(results_dir, 'performance_plot.png')
    plt.savefig(plot_path)
    plt.close()
    
    # Create detailed report
    report_path = os.path.join(results_dir, 'detailed_report.txt')
    with open(report_path, 'w') as f:
        f.write("Simulation Performance Report\n")
        f.write("=========================\n\n")
        f.write(f"Generated on: {datetime.now()}\n\n")
        
        for particle_count, result in results.items():
            f.write(f"\nParticle Count: {particle_count}\n")
            f.write(f"Duration: {result['duration']:.2f} seconds\n")
            f.write(f"Success: {result['success']}\n")
            if not result['success']:
                f.write(f"Error Output:\n{result['error']}\n")
    
    return results_dir

def main():
    # Define particle counts to test
    particle_counts = np.linspace(10,10000, num=10, dtype=int)
    results = {}
    
    print("Starting simulation performance testing")
    print("======================================")
    
    for count in particle_counts:
        print(f"\nRunning simulation with {count} particles...")
        
        # Update settings file
        if not update_json_settings(count):
            print(f"Failed to update settings for {count} particles, skipping...")
            continue
        
        # Run simulation
        print(f"Executing simulation...")
        result = run_simulation()
        results[count] = result
        
        if result['success']:
            print(f"Completed in {result['duration']:.2f} seconds")
        else:
            print(f"Failed! Check error output for details")
    
    # Generate performance report
    print("\nGenerating performance report...")
    report_dir = create_performance_report(results)
    print(f"Performance report generated in: {report_dir}")
    
    # Print summary
    print("\nSimulation Performance Summary:")
    print("==============================")
    for count in particle_counts:
        if count in results:
            status = "Success" if results[count]['success'] else "Failed"
            duration = results[count]['duration']
            print(f"{count} particles: {status} - {duration:.2f} seconds")

if __name__ == "__main__":
    main()

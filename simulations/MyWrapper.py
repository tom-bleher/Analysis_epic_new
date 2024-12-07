import json
import sys
from createSimulationFiles2 import HandleEIC

def load_pixel_sizes(json_path='pixel_data.json'):
    """Function to load pixel sizes from a JSON file."""
    try:
        with open(json_path, 'r') as file:
            pixel_data = json.load(file)
        return pixel_data.get('LumiSpecTracker_pixelSize', [])
    except Exception as e:
        print(f"Error loading pixel sizes: {e}")
        sys.exit(1)

def main():
    # Load pixel sizes from the JSON file
    pixel_sizes = load_pixel_sizes()
    for dx, dy in pixel_sizes:
        print(f"Running simulation for pixel size: {dx}x{dy}")
        
        # Create an instance of HandleEIC with the current pixel size
        eic = HandleEIC(pixel_sizes=[(dx, dy)])
        
        # Run the simulation for the current pixel size
        eic.main()

if __name__ == "__main__":
    main()

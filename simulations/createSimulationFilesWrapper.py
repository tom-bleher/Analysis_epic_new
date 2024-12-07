import subprocess
import time

fileType = "beamEffectsElectrons"
det_dir = "/data/tomble/eic/epic" 
epicPath = det_dir + "/install/share/epic/epic_ip6_extended.xml"
run_file = 'createSimFiles2.py' 

# read JSON and store in list
def setup_json() -> list[tuple]:
    """
    Method for setting up JSON file containing pixel size.
    If the JSON file doesn't exist or is incorrect, a new one is created with default pixel sizes.
    """
    px_size_dict = {}
    px_json_path = os.path.join(os.getcwd(), 'pixel_data.json')
    try:
        # try to read and load the JSON file
        with open(px_json_path,'r') as file:
            px_size_dict = json.load(file)
        # validate the read data
        px_data = px_size_dict['LumiSpecTracker_pixelSize']

        if all(isinstance(item, list) and len(item) == 2 and 
            isinstance(item[0], (float, int)) and isinstance(item[1], (float, int)) for item in px_data):
            px_pairs = px_data
        else:
            raise ValueError("Invalid JSON file...")
    except:
        # create a new JSON with default values
        px_size_dict = {"LumiSpecTracker_pixelSize": [[0.1,0.1]]}
        with open(px_json_path,'w') as file:
            json.dump(px_size_dict, file)
        print(f"No valid JSON found. Created JSON file at {px_json_path}. Edit the pairs in the JSON to specify your desired values.")
        px_pairs = px_size_dict['LumiSpecTracker_pixelSize']
    return px_pairs

def rewrite_xml_tree(det_dir, curr_px_dx, curr_px_dy) -> None:
    """
    Method for rewriting desired pixel values for all XML files of the Epic 
    detector. For every "{DETECTOR_PATH}" in copied epic XMLs, we replace with the path 
    for the current compact pixel path, and for every compact path 
    we replace with our new compact path 

    Args:
        curr_px_dx
        curr_px_dy
    """

    # iterate over all XML files in the copied epic directory
    for subdir, dirs, files in os.walk(det_dir):
        for filename in files:
            filepath = subdir + os.sep + filename
            if filepath.endswith(".xml") and os.access(filepath, os.W_OK):
                tree = ET.parse(filepath)
                root = tree.getroot()
                for elem in root.iter():
                    if "constant" in elem.tag and 'name' in elem.keys():
                        if elem.attrib['name'] == "LumiSpecTracker_pixelSize_dx":
                            elem.attrib['value'] = f"{curr_px_dx}*mm"
                        elif elem.attrib['name'] == "LumiSpecTracker_pixelSize_dy":
                            elem.attrib['value'] = f"{curr_px_dy}*mm"
                tree.write(filepath)

# Run the Python file
def run_python_file(file_path):
    try:
        result = subprocess.run(['python', file_path], capture_output=True, text=True, check=True)
        print(f"Output of the script:\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Error occurred while running {file_path}:\n{e.stderr}")

for curr_px_dx, curr_px_dy in setup_json():
    rewrite_xml_tree(det_dir, curr_px_dx, curr_px_dy)  # Update XML with current pixel values
    print(f"Running simulation for dx={curr_px_dx}, dy={curr_px_dy}...")
    run_python_file(run_file)  # Waits until the script finishes
    print(f"Simulation complete for dx={curr_px_dx}, dy={curr_px_dy}.")

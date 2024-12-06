import subprocess
import os
import xml.etree.ElementTree as ET

def rewrite_xml_tree(parent_dir, curr_px_dx, curr_px_dy):
    """
    Modify pixel values in XML files.
    """
    for subdir, dirs, files in os.walk(parent_dir):
        for filename in files:
            filepath = os.path.join(subdir, filename)
            if filepath.endswith(".xml"):
                tree = ET.parse(filepath)
                root = tree.getroot()
                for elem in root.iter():
                    if "constant" in elem.tag and 'name' in elem.attrib:
                        if elem.attrib['name'] == "LumiSpecTracker_pixelSize_dx":
                            elem.attrib['value'] = f"{curr_px_dx}*mm"
                        elif elem.attrib['name'] == "LumiSpecTracker_pixelSize_dy":
                            elem.attrib['value'] = f"{curr_px_dy}*mm"
                tree.write(filepath)

def run_python_file(file_path):
    """
    Run a Python file using subprocess asynchronously.
    """
    try:
        process = subprocess.Popen(['python3', file_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print(f"Subprocess started with PID: {process.pid}")
        # Do not wait; process runs in the background.
    except Exception as e:
        print(f"Failed to start subprocess for {file_path}: {e}")

def main():
    file_path = 'createSimFiles.py'
    run_python_file(file_path)  # Start subprocess (non-blocking)
    rewrite_xml_tree("/extra/tomble/eic", "0.1", "0.1")  # Runs immediately after subprocess starts

if __name__ == "__main__":
    main()

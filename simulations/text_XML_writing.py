


# copy detector for every pixel value 


# change every dx,dy, and $DETECTOR_PATH to 

# -*- coding: utf-8 -*-
"""
Created on Fri Sep  6 15:33:11 2024

@author: tombleher
"""
import os
import shutil
from datetime import datetime
import sys
import re
import json
import xml.etree.ElementTree as ET
import subprocess
import multiprocessing
import stat

def rewrite_xml_tree(curr_epic_path, curr_px_dx, curr_px_dy):
    """
    Rewrite the pixel values and replace ${DETECTOR_PATH} in all XML files 
    for the Epic detector.

    Args:
        curr_epic_path (str): Root directory containing XML files.
        curr_px_dx (float): New pixel size dx in mm.
        curr_px_dy (float): New pixel size dy in mm.
    """
    for subdir, dirs, files in os.walk(curr_epic_path):
        for filename in files:
            filepath = os.path.join(subdir, filename)
            if filepath.endswith(".xml"):
                try:
                    # Ensure the file is readable and writable
                    os.chmod(filepath, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)
                    
                    # Parse the XML file
                    tree = ET.parse(filepath)
                    root = tree.getroot()
                    
                    # Modify the XML
                    for elem in root.iter():
                        # Modify pixel size constants
                        if "constant" in elem.tag and 'name' in elem.attrib:
                            if elem.attrib['name'] == "LumiSpecTracker_pixelSize_dx":
                                elem.attrib['value'] = f"{curr_px_dx}*mm"
                            elif elem.attrib['name'] == "LumiSpecTracker_pixelSize_dy":
                                elem.attrib['value'] = f"{curr_px_dy}*mm"
                        
                        # Replace ${DETECTOR_PATH}
                        if elem.text and "${DETECTOR_PATH}" in elem.text:
                            elem.text = elem.text.replace("${DETECTOR_PATH}", curr_epic_path)
                    
                    # Save changes back to the file
                    tree.write(filepath)
                
                except Exception as e:
                    print(f"Error processing file {filepath}: {e}")

rewrite_xml_tree("/data/tomble/Analysis_epic_new/simulations/simEvents/20241206_214112/2.0x0.1px/epic_sim", "3.0", "0.5")
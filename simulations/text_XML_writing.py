


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

def rewrite_xml_tree(parent_dir, curr_px_dx, curr_px_dy):
    """
    Method for rewriting desired pixel values for all XML files of the Epic 
    detector. For every "{DETECTOR_PATH}" in copied epic XMLs, we replace with the path 
    for the current compact pixel path, and for every compact path 
    we replace with our new compact path 

    Args:
        parent_dir
        curr_px_dx
        curr_px_dy
    """
    # iterate over all XML files in the copied epic directory
    for subdir, dirs, files in os.walk(parent_dir):
        os.chmod(filepath, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)
        for filename in files:
            filepath = subdir + os.sep + filename
            if filepath.endswith(".xml"):
                tree = ET.parse(filepath)
                root = tree.getroot()
                for elem in root.iter():
                    if "constant" in elem.tag and 'name' in elem.keys():
                        if elem.attrib['name'] == "LumiSpecTracker_pixelSize_dx":
                            elem.attrib['value'] = f"{curr_px_dx}*mm"
                        elif elem.attrib['name'] == "LumiSpecTracker_pixelSize_dy":
                            elem.attrib['value'] = f"{curr_px_dy}*mm"
            
                    if elem.text and "${DETECTOR_PATH}" in elem.text:
                        elem.text = elem.text.replace("${DETECTOR_PATH}", curr_epic_path)
                        
                tree.write(filepath)

rewrite_xml_tree("/data/tomble/Analysis_epic_new/simulations/simEvents/20241206_212905/2.0x0.1px/epic_sim", "3.0", "0.5")
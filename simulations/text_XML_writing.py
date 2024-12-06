


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

def copy_epic():
    # copy epic to respective px folder for parameter reference 
    os.system('cp -r "/data/tomble/eic/epic_sim" data/tomble/Analysis_epic_new/')    
    return os.path.join("data/tomble/Analysis_epic_new/", "epic_sim")

def rewrite_xml_tree(self, curr_epic_path, curr_px_dx, curr_px_dy):
    """
    Method for rewriting desired pixel values for all XML files of the Epic 
    detector. For every "{DETECTOR_PATH}" in copied epic XMLs, we replace with the path 
    for the current compact pixel path, and for every compact path 
    we replace with our new compact path 

    Args:
        curr_epic_path
        curr_px_dx
        curr_px_dy
    """

    # iterate over all XML files in the copied epic directory
    for subdir, dirs, files in os.walk(curr_epic_path):
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
                    elif elem.text:
                        if "${DETECTOR_PATH}" in elem.text:
                            elem.text = elem.text.replace("${DETECTOR_PATH}", f"{curr_epic_path}")
                tree.write(filepath)

path = copy_epic()
rewrite_xml_tree(path, "3.0", "0.5")
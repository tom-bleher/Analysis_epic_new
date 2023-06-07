#!/bin/bash

# Source this script!

source /opt/detector/setup.sh
source epic/install/setup.sh
export EICrecon_MY=/group/eic/users/sjdkay/ePIC/EICrecon_MY
source EICrecon/bin/eicrecon-this.sh
export JANA_PLUGIN_PATH=$JANA_PLUGIN_PATH:/usr/local/lib/EICrecon/plugins 

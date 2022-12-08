#! /bin/bash

alias root='root --web=off'

source /opt/detector/setup.sh
source $HOME/eic/epic/install/setup.sh
export EICrecon_MY=$HOME/eic/EICrecon_MY
export JANA_PLUGIN_PATH=$JANA_PLUGIN_PATH:/usr/local/lib/EICrecon/plugins

#!/bin/bash
#set -e  # Exit on any error
#set -o pipefail  # Pipe failures are treated as errors

# Input parameters
DETECTOR_PATH="$1"
DETECTOR_PARENT="$2"
EXECUTION_PATH="$3"
SIM_OUT_PATH="$4"
SIF_PATH="$5"
SIM_CMD="$6"
DO_RECON="${7:-false}"  # Optional reconstruction flag
PLUGIN_PATH="${8:-}"    # Optional plugin path for reconstruction

echo "Starting with parameters:"
echo "DETECTOR_PATH: $DETECTOR_PATH"
echo "DETECTOR_PARENT: $DETECTOR_PARENT"
echo "EXECUTION_PATH: $EXECUTION_PATH"
echo "SIM_OUT_PATH: $SIM_OUT_PATH"
echo "DO_RECON: $DO_RECON"
[[ "$DO_RECON" == "true" ]] && echo "PLUGIN_PATH: $PLUGIN_PATH"

# Verify required parameters
if [ -z "$DETECTOR_PATH" ] || [ -z "$SIF_PATH" ] || [ -z "$SIM_CMD" ]; then
    echo "Error: Missing required parameters"
    exit 1
fi

# Additional validation for reconstruction
if [ "$DO_RECON" == "true" ] && [ -z "$PLUGIN_PATH" ]; then
    echo "Error: Plugin path required for reconstruction"
    exit 1
fi

# Check if files/directories exist
if [ ! -f "$SIF_PATH" ]; then
    echo "Error: Singularity image not found at $SIF_PATH"
    exit 1
fi

if [ ! -d "$DETECTOR_PATH" ]; then
    echo "Error: Detector directory not found at $DETECTOR_PATH"
    exit 1
fi

# Configure bind paths
BIND_PATHS="$DETECTOR_PARENT:$DETECTOR_PARENT,$EXECUTION_PATH:$EXECUTION_PATH"
if [ ! -z "$SIM_OUT_PATH" ]; then
    BIND_PATHS="$BIND_PATHS,$SIM_OUT_PATH:$SIM_OUT_PATH"
fi
if [ "$DO_RECON" == "true" ]; then
    BIND_PATHS="$BIND_PATHS,$PLUGIN_PATH:$PLUGIN_PATH"
fi

# Extract output file from simulation command for reconstruction
if [ "$DO_RECON" == "true" ]; then
    echo "Extracting paths for reconstruction..."
    SIM_OUTPUT=$(echo "$SIM_CMD" | grep -o '\-\-outputFile [^ ]*' | cut -d' ' -f2)
    RECON_DIR="$(dirname "${SIM_OUTPUT}")/recon"
    RECON_OUTPUT="${RECON_DIR}/recon_$(basename ${SIM_OUTPUT})"
    
    # Create reconstruction directory
    mkdir -p "${RECON_DIR}"
    echo "Created reconstruction directory: ${RECON_DIR}"
    echo "Will write reconstruction output to: ${RECON_OUTPUT}"
fi

# Run simulation and optional reconstruction inside Singularity
singularity exec --containall \
    --bind ${BIND_PATHS} \
    ${SIF_PATH} /bin/bash << EOF
set -e
set -x

cd ${DETECTOR_PATH}
echo "Current working directory: \$(pwd)"

# Source the environment
source install/bin/thisepic.sh || { echo "Failed to source local environment"; exit 1; }

# Show environment for debugging
echo "PATH=\$PATH"
echo "LD_LIBRARY_PATH=\$LD_LIBRARY_PATH"

# Run simulation
echo "Running simulation command: ${SIM_CMD}"
${SIM_CMD}

# Run reconstruction if requested
if [ "$DO_RECON" == "true" ]; then
    echo "Setting up reconstruction environment"
    
    # Set up correct plugin paths using EIC shell defaults
    export EICRECON_PLUGIN_PATH="/opt/local/lib/EICrecon/plugins"
    export EICrecon_MY="${PLUGIN_PATH}/EICrecon_MY"
    # Use the same plugin path as EIC shell
    export JANA_PLUGIN_PATH="/opt/local/lib/EICrecon/plugins:/opt/local/plugins:${EICrecon_MY}"
    
    echo "Running reconstruction"
    echo "Input: ${SIM_OUTPUT}"
    echo "Output: ${RECON_OUTPUT}"
    echo "Plugin paths:"
    echo "EICRECON_PLUGIN_PATH=${EICRECON_PLUGIN_PATH}"
    echo "EICrecon_MY=${EICrecon_MY}"
    echo "JANA_PLUGIN_PATH=${JANA_PLUGIN_PATH}"
    
    # Add required paths to bind path
    BIND_PATHS="${BIND_PATHS},/opt/local:/opt/local"
    
    # Ensure output directory exists
    mkdir -p "$(dirname "${RECON_OUTPUT}")"
    
    # Add more debugging information
    echo "Current directory: $(pwd)"
    ls -l "${SIM_OUTPUT}" || echo "Warning: Cannot access simulation output"
    echo "DETECTOR_PATH=${DETECTOR_PATH}"
    
    # Run reconstruction
    cd $(dirname "${SIM_OUTPUT}")  # Change to directory containing input file
    eicrecon -Pplugins=analyzeLumiHits \
        -Phistsfile="${RECON_OUTPUT}" \
        "${SIM_OUTPUT}" || { 
            echo "Reconstruction failed"
            echo "Contents of recon directory:"
            ls -l "${RECON_DIR}"
            exit 1; 
        }
    
    # Verify reconstruction output was created
    if [ ! -f "${RECON_OUTPUT}" ]; then
        echo "Error: Reconstruction output file was not created"
        exit 1
    fi
    
    echo "Reconstruction completed successfully"
    echo "Output file details:"
    ls -l "${RECON_OUTPUT}"
fi

EOF

# Exit with success
exit 0
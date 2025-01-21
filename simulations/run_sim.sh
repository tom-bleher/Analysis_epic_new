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

# Source the environment with error checking
if [ ! -f "install/bin/thisepic.sh" ]; then
    echo "Error: thisepic.sh not found in install/bin/"
    exit 1
fi

source install/bin/thisepic.sh || {
    echo "Failed to source environment"
    exit 1
}

# Verify ddsim is available
if ! command -v ddsim &> /dev/null; then
    echo "Error: ddsim command not found"
    echo "PATH=\$PATH"
    exit 1
fi

# Show environment for debugging
echo "PATH=\$PATH"
echo "LD_LIBRARY_PATH=\$LD_LIBRARY_PATH"
echo "Which ddsim: \$(which ddsim)"

# Extract simulation output file path and create directory
SIM_OUTPUT=$(echo "$SIM_CMD" | grep -o '\-\-outputFile [^ ]*' | cut -d' ' -f2)
mkdir -p "$(dirname "${SIM_OUTPUT}")"

# Run simulation with progress monitoring
echo "Starting simulation command: ${SIM_CMD}"
echo "Output will be written to: ${SIM_OUTPUT}"
cd "$(dirname "${SIM_OUTPUT}")"

# Create temporary files for output capture
ERROR_LOG=$(mktemp)
STDOUT_LOG=$(mktemp)

# Add --runType batch and verbose logging if not already present
if [[ ! "${SIM_CMD}" =~ "--runType" ]]; then
    SIM_CMD="${SIM_CMD} --runType batch"
fi
if [[ ! "${SIM_CMD}" =~ "-v" ]] && [[ ! "${SIM_CMD}" =~ "--printLevel" ]]; then
    SIM_CMD="${SIM_CMD} -v DEBUG"
fi

echo "Modified simulation command: ${SIM_CMD}"

# Run the simulation with output capture
if ! ${SIM_CMD} > >(tee "${STDOUT_LOG}") 2> >(tee "${ERROR_LOG}" >&2); then
    echo "Error: Simulation command failed"
    echo "Environment:"
    env | grep -E "^(LD_LIBRARY_PATH|PATH|DD4HEP)"
    echo "Current directory: $(pwd)"
    echo "Directory contents:"
    ls -la
    echo "Last 50 lines of stdout:"
    tail -n 50 "${STDOUT_LOG}"
    echo "Error log:"
    cat "${ERROR_LOG}"
    rm -f "${ERROR_LOG}" "${STDOUT_LOG}"
    exit 1
fi

# Verify simulation output
if [ ! -f "${SIM_OUTPUT}" ]; then
    echo "Error: Simulation output file not created: ${SIM_OUTPUT}"
    echo "Directory contents:"
    ls -la "$(dirname "${SIM_OUTPUT}")"
    echo "Last 50 lines of simulation output:"
    tail -n 50 "${STDOUT_LOG}"
    rm -f "${ERROR_LOG}" "${STDOUT_LOG}"
    exit 1
fi

# Check file size with more detailed error
SIM_SIZE=$(stat -f%z "${SIM_OUTPUT}" 2>/dev/null || stat -c%s "${SIM_OUTPUT}")
if [ "$SIM_SIZE" -lt 1000 ]; then
    echo "Error: Simulation output file too small: ${SIM_OUTPUT} (${SIM_SIZE} bytes)"
    echo "This usually indicates a simulation failure. Checking logs:"
    echo "Last 50 lines of stdout:"
    tail -n 50 "${STDOUT_LOG}"
    echo "Error log:"
    cat "${ERROR_LOG}"
    rm -f "${ERROR_LOG}" "${STDOUT_LOG}"
    exit 1
fi

# Cleanup
rm -f "${ERROR_LOG}" "${STDOUT_LOG}"

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
    
    # Run reconstruction and check output
    eicrecon -Pplugins=analyzeLumiHits \
        -Phistsfile="${RECON_OUTPUT}" \
        "${SIM_OUTPUT}" || { 
            echo "Reconstruction failed"
            echo "Contents of recon directory:"
            ls -l "${RECON_DIR}"
            exit 1
        }
    
    # Verify reconstruction output
    if [ ! -f "${RECON_OUTPUT}" ]; then
        echo "Error: Reconstruction output file not created: ${RECON_OUTPUT}"
        exit 1
    fi
    
    # Check reconstruction output file size
    RECON_SIZE=$(stat -f%z "${RECON_OUTPUT}" 2>/dev/null || stat -c%s "${RECON_OUTPUT}")
    if [ "$RECON_SIZE" -lt 1000 ]; then
        echo "Error: Reconstruction output file too small: ${RECON_OUTPUT} (${RECON_SIZE} bytes)"
        exit 1
    fi
    
    echo "Reconstruction completed successfully"
    echo "Output file details:"
    ls -l "${RECON_OUTPUT}"
fi

EOF

# Exit with success
exit 0
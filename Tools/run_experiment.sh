#!/bin/bash

# 
# This script run the experiment for a wide range of values
# it loads directly the traces to the analysis folder, where they belong
#

# Experiment configuration
EXPERIMENT_EXECUTABLE="hello" # Name of the binary file to execute (without .bin extension)
EXPERIMENT_CONFIGURATION="hello" # Name of the configuration file to load (without .conf extension)

# General configuration 
DATA_ANALYSIS_DIR="../analysis/data"

### Let's start with Cluster - 10
function ExecuteExperiment {
    NAME=$1 
    BYZ_VALUES=$2
    TOPOLOGY=$3
    DOCKER_CONTAINER=$4

    LOCAL_TRACE_DIR="$DATA_ANALYSIS_DIR/$TOPOLOGY"

    for BYZ_VALUE in "${BYZ_VALUES[@]}" do 
        echo "- - $BYZ_VALUE"
        LOCAL_TRACE="$LOCAL_TRACE_DIR/$NAME-$BYZ_VALUE-$EXPERIMENT_CONFIGURATION.trace"
        CONTAINER_TRACE="/home/experiment/experiments/traces/tracefile.trace"
        docker run -v "$LOCAL_TRACE:$CONTAINER_TRACE" experiment "$EXPERIMENT_EXECUTABLE" "$EXPERIMENT_CONFIGURATION" "$TOPOLOGY"
    done
}

### HERE MARS
echo "Executing MARS experiments"
DOCKER_CONTAINER="experiment" # Put here the name of the container to run the experiment
echo "- Cluster-10"
ExecuteExperiment "MARS" array(0, 10, 20, 30) "Cluster-10" $DOCKER_CONTAINER
echo "- Cluster-100"
ExecuteExperiment "MARS" array(0, 1, 2, 3, 4, 5, 10, 15, 20, 25, 30) "Cluster-100" $DOCKER_CONTAINER

### HERE Blockchain

### HERE HADOOP STANDARD
#!/bin/bash

# 
# This script run the experiment for a wide range of values
# it loads directly the traces to the analysis folder, where they belong
#

# Experiment configuration
EXPERIMENT_EXECUTABLE="hello" # Name of the binary file to execute (without .bin extension)
EXPERIMENT_CONFIGURATIONS=("sum" "sumlowblocktime" "sumzeroblocktime") # Name of the configuration file to load (without .conf extension)

# General configuration 
DATA_ANALYSIS_DIR="$PWD/../analysis/data"

BYZ_VALUES=(0 10 20 30 40)
NO_BYZ_VALUES=(0)

TMP_TRACES_DIR="$PWD/traces.tmp"
mkdir $TMP_TRACES_DIR

### Let's start with Cluster - 10
function ExecuteExperiment {
    NAME=$1 
    BYZ_VALUES=$2
    TOPOLOGY=$3
    DOCKER_CONTAINER=$4

    LOCAL_TRACE_DIR="$DATA_ANALYSIS_DIR/$TOPOLOGY"
    LOCAL_CONFIGURATION="$PWD/configurations"

    for BYZ_VALUE in "${BYZ_VALUES[@]}" 
    do         
        echo "--- $BYZ_VALUE"
        CONTAINER_TRACE="/home/experiment/experiments/traces/"
        CONTAINER_CONFIG="/home/experiment/experiments/configurations/"

        for CONFIGURATION in "${EXPERIMENT_CONFIGURATIONS[@]}" 
        do         
            echo "...... $CONFIGURATION"
            docker run --rm -v "$TMP_TRACES_DIR:$CONTAINER_TRACE" -v "$LOCAL_CONFIGURATION:$CONTAINER_CONFIG" $DOCKER_CONTAINER "$EXPERIMENT_EXECUTABLE" "$CONFIGURATION" "$TOPOLOGY" "$BYZ_VALUE"
            mv "$TMP_TRACES_DIR/tracefile.trace" "$LOCAL_TRACE_DIR/$NAME-$BYZ_VALUE-$CONFIGURATION.trace"
        done
    done
}

### HERE MARS
# echo "Executing MARS SINGLE experiments"
# DOCKER_CONTAINER="marssim:single" # Put here the name of the container to run the experiment
# echo "- Cluster-10"
# ExecuteExperiment "MARS_S" "$BYZ_VALUES" "Cluster-10" $DOCKER_CONTAINER
# echo "- Cluster-100"
# BYZ_VALUES=(0 1 2 3 4 5 10 15 20 25 30 40 50 60 80 100)
# ExecuteExperiment "MARS_S" "$BYZ_VALUES" "Cluster-100" $DOCKER_CONTAINER

### HERE MARS
echo "Executing MARS ALL TASKS experiments"
DOCKER_CONTAINER="marssim:all" # Put here the name of the container to run the experiment
echo "- Cluster-10"
ExecuteExperiment "MARS_M" "$BYZ_VALUES" "Cluster-10" $DOCKER_CONTAINER
# echo "- Cluster-100"
# BYZ_VALUES=(0 1 2 3 4 5 10 15 20 25 30 40 50 60 80 100)
# ExecuteExperiment "MARS_M" "$BYZ_VALUES" "Cluster-100" $DOCKER_CONTAINER

### HERE Blockchain
echo "Executing Blockchain experiments"
DOCKER_CONTAINER="blockchainsim"
echo "- Cluster-10"
ExecuteExperiment "BLOCKCHAIN" "$BYZ_VALUES" "Cluster-10" $DOCKER_CONTAINER
# echo "- Cluster-100"
# BYZ_VALUES=(0 1 2 3 4 5 10 15 20 25 30 40 50 60 80 100)
# ExecuteExperiment "BLOCKCHAIN" "$BYZ_VALUES" "Cluster-100" $DOCKER_CONTAINER

### HERE HADOOP STANDARD
echo "Executing Hadoop Standard experiments"
DOCKER_CONTAINER="hadoopsim"
echo "- Cluster-10"
ExecuteExperiment "HADOOP" "$NO_BYZ_VALUES" "Cluster-10" $DOCKER_CONTAINER
# echo "- Cluster-100"
# ExecuteExperiment "HADOOP" "$BYZ_VALUES" "Cluster-100" $DOCKER_CONTAINER

### HERE HADOOP BFT
echo "Executin Hadoop BFT experiments"
DOCKER_CONTAINER="hadoopbftsim"
echo "- Cluster-10"
ExecuteExperiment "HADOOP_BFT" "$BYZ_VALUES" "Cluster-10" $DOCKER_CONTAINER
# echo "- Cluster-100"
# BYZ_VALUES=(0 1 2 3 4 5 10 15 20 25 30 40 50 60 80 100)
# ExecuteExperiment "HADOOP_BFT" "$BYZ_VALUES" "Cluster-100" $DOCKER_CONTAINER

rm -r "$TMP_TRACES_DIR"

echo " "
echo "######  Preparing traces for data Analysis ######"
echo ""

cd ../analysis/data/
./prepare.sh

echo "\n\nFinished `date`"

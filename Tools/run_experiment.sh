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

#Configuration Byzantine-Clusters
declare -A CLUSTERS
BYZ_VALUES_SMALL=(0 10 20 30 40)
CLUSTERS["Cluster-10"]=$BYZ_VALUES_SMALL
BYZ_VALUES_LARGE=(0 1 5 10 20 30 40)
CLUSTERS["Cluster-100"]=$BYZ_VALUES_LARGE

TMP_TRACES_DIR="$PWD/traces.tmp"
mkdir $TMP_TRACES_DIR

### Let's start with Cluster - 10
function ExecuteExperiment {
    NAME=$1 
    DOCKER_CONTAINER=$2
    
    for TOPOLOGY in "${!CLUSTERS[@]}"
    do
        echo "Topology - " $TOPOLOGY
        BYZ_VALUES=${CLUSTERS[$TOPOLOGY]}            

        LOCAL_TRACE_DIR="$DATA_ANALYSIS_DIR/$TOPOLOGY"
        LOCAL_CONFIGURATION="$PWD/configurations"

        for BYZ_VALUE in "${BYZ_VALUES[@]}" 
        do         
            echo "--- $BYZ_VALUE %"
            CONTAINER_TRACE="/home/experiment/experiments/traces/"
            CONTAINER_CONFIG="/home/experiment/experiments/configurations/"

            for CONFIGURATION in "${EXPERIMENT_CONFIGURATIONS[@]}" 
            do         
                echo "...... $CONFIGURATION"
                docker run --rm -v "$TMP_TRACES_DIR:$CONTAINER_TRACE" -v "$LOCAL_CONFIGURATION:$CONTAINER_CONFIG" $DOCKER_CONTAINER "$EXPERIMENT_EXECUTABLE" "$CONFIGURATION" "$TOPOLOGY" "$BYZ_VALUE"
                mv "$TMP_TRACES_DIR/tracefile.trace" "$LOCAL_TRACE_DIR/$NAME-$BYZ_VALUE-$CONFIGURATION.trace"
            done
        done
    done
}
echo " "
echo "######  Preparation  ######"
echo ""
# Clean old traces
echo "Cleaning old traces"
"$DATA_ANALYSIS_DIR/cleantraces.sh"

echo " "
echo "######  Experiment  ######"
echo ""

### HERE MARS
echo "Executing MARS ALL TASKS experiments"
ExecuteExperiment "MARS_M" "marssim:byzantines"    

### HERE Blockchain
echo "Executing Blockchain experiments"
ExecuteExperiment "BLOCKCHAIN" "blockchainsim"

### HERE HADOOP STANDARD
echo "Executing Hadoop Standard experiments"
ExecuteExperiment "HADOOP" "hadoopsim"

### HERE HADOOP BFT
echo "Executin Hadoop BFT experiments"
ExecuteExperiment "HADOOP_BFT" "hadoopbftsim:byzantines"

rm -r "$TMP_TRACES_DIR"

echo " "
echo "######  Preparing traces for data Analysis  ######"
echo ""

cd ../analysis/data/
# ./prepare.sh Old slow method
python3 prepare.py

echo " "
echo "######  Terminated  ######"
echo ""

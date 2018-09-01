#!/bin/bash

# 
# This script run the experiment for a wide range of values
# it loads directly the traces to the analysis folder, where they belong
#

# Experiment configuration
EXPERIMENT_EXECUTABLE="hello" # Name of the binary file to execute (without .bin extension)

# General configuration 
DATA_ANALYSIS_DIR="$PWD/../analysis/data"
TOOL_DIR="$PWD"

#Configuration Byzantine-Clusters
declare -A CLUSTERS
# BYZ_VALUES_SMALL="0 10 20 30 40"
# CLUSTERS["Cluster-10"]=$BYZ_VALUES_SMALL
BYZ_VALUES_LARGE="0 1 5 10 20 30 40"
CLUSTERS["Cluster-100"]=$BYZ_VALUES_LARGE
SEEDS="0"

TMP_TRACES_DIR="$PWD/traces.tmp"
mkdir $TMP_TRACES_DIR

function ExecuteExperiment {
    NAME=$1 
    DOCKER_CONTAINER=$2

    for TOPOLOGY in "${!CLUSTERS[@]}"
    do
        echo "Topology - " $TOPOLOGY
        BYZ_VALUES=${CLUSTERS[$TOPOLOGY]}            

        LOCAL_TRACE_DIR="$DATA_ANALYSIS_DIR/$TOPOLOGY"
        LOCAL_CONFIGURATION="$PWD/configurations"

        for BYZ_VALUE in $BYZ_VALUES 
        do         
            echo "--- $BYZ_VALUE %"
            CONTAINER_TRACE="/home/experiment/experiments/traces/"
            CONTAINER_CONFIG="/home/experiment/experiments/configurations/"

            for CONFIGURATION in "${EXPERIMENT_CONFIGURATIONS[@]}" 
            do         
                echo "...... $CONFIGURATION"
                for SEED in $SEEDS
                do
                    echo "............... $SEED"
                    docker run --rm -v "$TMP_TRACES_DIR:$CONTAINER_TRACE" -v "$LOCAL_CONFIGURATION:$CONTAINER_CONFIG" $DOCKER_CONTAINER "$EXPERIMENT_EXECUTABLE" "$CONFIGURATION" "$TOPOLOGY" "$BYZ_VALUE" "$SEED" 
                    mv "$TMP_TRACES_DIR/tracefile.trace" "$LOCAL_TRACE_DIR/$NAME-$BYZ_VALUE-$CONFIGURATION-$SEED.trace"
                done
            done
        done
    done
}
echo " "
echo "######  Preparation  ######"
echo ""
# Clean old traces
# echo "Cleaning old traces"
# cd "$DATA_ANALYSIS_DIR"
# ./cleantraces.sh
# cd "$TOOL_DIR"

echo " "
echo "######  Experiment  ######"
echo ""

### HERE MARS
EXPERIMENT_CONFIGURATIONS=("sum" "sumlowblocktime" "sumzeroblocktime" "sum_no_real_byz" "sumlowblocktime_no_real_byz" "sumzeroblocktime_no_real_byz")
echo "Executing MARS ALL TASKS experiments"
SEEDS="0 2 5"
ExecuteExperiment "MARS_M" "marssim:byzantines"  
SEEDS="0"  
echo "Executing MARS PESSIMISTIC experiments"
ExecuteExperiment "MARS_M_PESSIMISTIC" "marssim:pessimistic"    


### HERE Blockchain

EXPERIMENT_CONFIGURATIONS=("sum" "sumlowblocktime" "sumzeroblocktime") 
echo "Executing Blockchain experiments"
SEEDS="0"
ExecuteExperiment "BLOCKCHAIN" "blockchainsim"


### HERE HADOOP BFT
EXPERIMENT_CONFIGURATIONS=("sum" "sum_no_real_byz") # Configurations without the block time variations
echo "Executin Hadoop BFT experiments"
SEEDS="0 2 5"
ExecuteExperiment "HADOOP_BFT" "hadoopbftsim:byzantines"

### HERE HADOOP STANDARD
EXPERIMENT_CONFIGURATIONS=("sum")
echo "Executing Hadoop Standard experiments"
SEEDS="0"
BYZ_VALUES_ZERO="0"
CLUSTERS["Cluster-10"]=$BYZ_VALUES_ZERO
CLUSTERS["Cluster-100"]=$BYZ_VALUES_ZERO
ExecuteExperiment "HADOOP" "hadoopsim"

rm -r "$TMP_TRACES_DIR"

echo " "
echo "######  Preparing traces for data Analysis  ######"
echo ""

cd ../analysis/data/
# ./prepare.sh # Old slow method
python3 prepare.py

echo " "
echo "######  Terminated  ######"
echo ""

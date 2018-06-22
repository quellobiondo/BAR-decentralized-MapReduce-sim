#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Usage error: <Experiment_Executable_Name> <Experiment_Configuration_Name> <Experiment_Topology_Name>"
    exit 1
fi

EXPERIMENT_EXECUTABLE=$1
EXPERIMENT_CONFIGURATION=$2
EXPERIMENT_TOPOLOGY=$3

EXPERIMENT_PLATFORM=""
if  [[ $EXPERIMENT_TOPOLOGY == Cluster-* ]];
then
    EXPERIMENT_PLATFORM="g5k.xml"
elif [[ $EXPERIMENT_TOPOLOGY == P2P-* ]];
then 
    EXPERIMENT_PLATFORM="vivaldi.xml"
else
    echo "Topology without a platform $EXPERIMENT_TOPOLOGY"
    exit 1
fi

echo "Next-op: bin/$EXPERIMENT_EXECUTABLE.bin configuratons/$EXPERIMENT_CONFIGURATION.conf deployments/$EXPERIMENT_TOPOLOGY platforms/$EXPERIMENT_PLATFORM"

#Execute the experiment
"bin/$EXPERIMENT_EXECUTABLE.bin" "configuratons/$EXPERIMENT_CONFIGURATION.conf" "deployments/$EXPERIMENT_TOPOLOGY" "platforms/$EXPERIMENT_PLATFORM"
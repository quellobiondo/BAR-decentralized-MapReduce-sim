#!/bin/bash

if [ "$#" -ne 4 ]; then
    echo "Usage error: <Experiment_Executable_Name> <Experiment_Configuration_Name> <Experiment_Topology_Name> <Byzantine Values>"
    exit 1
fi

EXPERIMENT_EXECUTABLE=$1
EXPERIMENT_CONFIGURATION=$2
EXPERIMENT_TOPOLOGY=$3
BYZ_VALUE=$4

CONF_FILE="configurations/$EXPERIMENT_CONFIGURATION.conf"
DEPL_FILE="deployments/$EXPERIMENT_TOPOLOGY.deploy.xml"
BIN_FILE="bin/$EXPERIMENT_EXECUTABLE.bin"
#
# Edit configuration and set Experiment Platform
#
sed -i -r "s/byzantine [0-9]+/byzantine $BYZ_VALUE/g" "$CONF_FILE"

EXPERIMENT_PLATFORM=""
if  [[ $EXPERIMENT_TOPOLOGY == Cluster-* ]];
then
    EXPERIMENT_PLATFORM="platforms/g5k.xml"
elif [[ $EXPERIMENT_TOPOLOGY == P2P-* ]];
then 
    EXPERIMENT_PLATFORM="platforms/vivaldi.xml"
else
    echo "Topology without a platform $EXPERIMENT_TOPOLOGY"
    exit 1
fi

echo "Next-op: $BIN_FILE $CONF_FILE $DEPL_FILE $EXPERIMENT_PLATFORM"

#Execute the experiment
"$BIN_FILE" "$CONF_FILE" "$DEPL_FILE" "$EXPERIMENT_PLATFORM"
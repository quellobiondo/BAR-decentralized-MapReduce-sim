#! /bin/bash

TOPOLOGIES=("Cluster-10" "Cluster-100" "P2P-10" )
COMPLETION_FILE_NAME="$PWD/completion_time.csv"
CPU_USAGE_FILE_NAME="$PWD/cpu_usage.csv"

function createTmpFile {
    if  [ -f "$1" ]; then
        rm "$1"
    fi
    touch "$1"
}

function processRawPaje {
    # Extracting all intermediate files necessary for processing
    local PJ_DUMP=$(pj_dump --ignore-incomplete-links "$1")

    STATE=$(echo "$PJ_DUMP" | grep State)
    VARIABLE=$(echo "$PJ_DUMP" | grep Variable)
    LINK=$(echo "$PJ_DUMP" | grep Link)
    CONTAINER=$(echo "$PJ_DUMP" | grep Container)

    echo "$PJ_DUMP" > pj_dump.csv
    echo "$STATE" > pj_dump_state.csv
}

# 
# Creation of completion time table
# 
function createCompletionTimeTable {
    if [ ! -f "$1" ]; then
        echo "Topology, NumberOfNodes, Platform, Byzantine, MapDuration, ReduceDuration, TotalDuration, Updated, Config" > "$1"
    fi
}

# 
# Creation of CPU usage table
# 
function createCPUUsageTable {
    if [ ! -f "$1" ]; then
        echo "Topology, NumberOfNodes, Platform, Byzantine, Power, Time, Updated, Config" > "$1"
    fi
}


echo "Creating completion time table"
rm "$COMPLETION_FILE_NAME" "$CPU_USAGE_FILE_NAME"
createCompletionTimeTable "$COMPLETION_FILE_NAME"
createCPUUsageTable "$CPU_USAGE_FILE_NAME"

for TOPOLOGY_COMPLETE in "${TOPOLOGIES[@]}"
do
    TOPOLOGY="${TOPOLOGY_COMPLETE%-*}"
    NUMBER_OF_NODES="${TOPOLOGY_COMPLETE#*-}"

    echo "Processing $TOPOLOGY_COMPLETE"
    cd "$TOPOLOGY_COMPLETE"
    
    for RAW_PAJE in *.trace; do 
        
        if [[ -e $RAW_PAJE ]]; then
            processRawPaje "$RAW_PAJE"

            NAME=${RAW_PAJE%.*}
            
            echo "- $NAME"

            PLATFORM=${NAME%%-*}
            T=${NAME%-*}
            BYZANTINE_PERC=${T##*-}
            CONFIGURATION=${NAME##*-}

            # StateTraceFile="state.$NAME.csv"
            # VariableTraceFile="variable.$NAME.csv"
            # LinkTraceFile="link.$NAME.csv"
            # ContainerTraceFile="container.$NAME.csv"

            ## Compute stats for Job Duration
            echo "... computing Job duration"            

            MAP_BEGIN=0
            # MAP_END=$(grep -e "MAP,.*END" -m 1 $StateTraceFile | cut -d',' -f 4)
            # REDUCE_BEGIN=$(grep -e "REDUCE,.*START" -m 1 $StateTraceFile | cut -d',' -f 4)
            # REDUCE_END=$(grep -e "REDUCE,.*END" -m 1 $StateTraceFile | cut -d',' -f 4)

            MAP_END=$(echo "$STATE" | grep -e "MAP,.*END" -m 1 | cut -d',' -f 4)
            REDUCE_BEGIN=$(echo "$STATE" | grep -e "REDUCE,.*START" -m 1 | cut -d',' -f 4)
            REDUCE_END=$(echo "$STATE" | grep -e "REDUCE,.*END" -m 1 | cut -d',' -f 4)

            MAP_DURATION=`awk "BEGIN {printf \"%.2f\n\", $MAP_END-$MAP_BEGIN}"`
            REDUCE_DURATION=`awk "BEGIN {printf \"%.2f\n\", $REDUCE_END-$REDUCE_BEGIN}"`
            TOTAL_DURATION=`awk "BEGIN {printf \"%.2f\n\", $REDUCE_END-$MAP_BEGIN}"`
            
            echo "$TOPOLOGY, $NUMBER_OF_NODES, $PLATFORM, $BYZANTINE_PERC, $MAP_DURATION, $REDUCE_DURATION, $TOTAL_DURATION, TRUE, $CONFIGURATION\n" >> "$COMPLETION_FILE_NAME"

            ## Compute stats for CPU consumption
            echo "... computing CPU consumption"
            BUFFER=""
            echo "$VARIABLE" | grep "power_used" | while read -r line ; do
                TIME=$(echo $line | cut -d',' -f 6)
                POWER=$(echo $line | cut -d',' -f 7)
                BUFFER="$BUFFER$TOPOLOGY, $NUMBER_OF_NODES, $PLATFORM, $BYZANTINE_PERC, $POWER, $TIME, TRUE, $CONFIGURATION\n"                
            done
            echo "$BUFFER" >> "$CPU_USAGE_FILE_NAME"
        fi
    done
    cd .. 
done
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

    NAME=${1//.trace/.csv}
    TMP_FILE="pjdump.out.tmp"
    createTmpFile $TMP_FILE
    pj_dump --ignore-incomplete-links $1 > $TMP_FILE
    grep State $TMP_FILE > "state.$NAME"
    grep Variable $TMP_FILE > "variable.$NAME"
    grep Link $TMP_FILE > "link.$NAME"
    grep Container $TMP_FILE > "container.$NAME"
    rm $TMP_FILE
}

# Useful functions for later

function AppendCSVLine {
    FILE_NAME="$1"
    shift 1

    INDEX=0
    for VALUE in $@; do
        if [ $INDEX -gt 0 ]; then
            printf "," >> "$FILE_NAME"
        fi
        printf "$VALUE" >> "$FILE_NAME"
        INDEX=1
    done

    printf "\n" >> "$FILE_NAME"
}

function eraseOldDataInTable {
    TMPFILENAME=$(mktemp /tmp/MARS-prepare-script.XXXXXX)
    
    ORIGINAL_FILE="$1"
    TOPOLOGY="$2"
    NUMBER_OF_NODES="$3"
    PLATFORM="$4"
    BYZANTINE_PERC="$5"
    CONFIGURATION="$6"

    ## Erase old data
    grep -vwE "$TOPOLOGY,$NUMBER_OF_NODES,$PLATFORM,$BYZANTINE_PERC,.*,$CONFIGURATION" "$ORIGINAL_FILE" > "$TMPFILENAME"
    mv "$TMPFILENAME" "$ORIGINAL_FILE"
}

# 
# Creation of completion time table
# 
function createCompletionTimeTable {
    if [ ! -f "$1" ]; then
        touch "$1"
        AppendCSVLine "$1" "Topology" "NumberOfNodes" "Platform" "Byzantine" "MapDuration" "ReduceDuration" "TotalDuration" "Updated" "Config"
    fi
}

# 
# Creation of CPU usage table
# 
function createCPUUsageTable {
    if [ ! -f "$1" ]; then
        touch "$1"
        AppendCSVLine "$1" "Topology" "NumberOfNodes" "Platform" "Byzantine" "Power" "Time" "Updated" "Config"
    fi
}


echo "Creating completion time table"
rm "$COMPLETION_FILE_NAME" "$CPU_USAGE_FILE_NAME"
createCompletionTimeTable "$COMPLETION_FILE_NAME"
createCPUUsageTable "$CPU_USAGE_FILE_NAME"

TMP_FILE_DURATION="$PWD/filedurations.tmp"
TMP_FILE_CPU="$PWD/filecpu.tmp"
createTmpFile "$TMP_FILE_DURATION"
createTmpFile "$TMP_FILE_CPU"

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

            StateTraceFile="state.$NAME.csv"
            VariableTraceFile="variable.$NAME.csv"
            LinkTraceFile="link.$NAME.csv"
            ContainerTraceFile="container.$NAME.csv"

            ## Compute stats for Job Duration
            echo "... computing Job duration"
            
            # Erase old data
            #eraseOldDataInTable $COMPLETION_FILE_NAME $TOPOLOGY $NUMBER_OF_NODES $PLATFORM $BYZANTINE_PERC $CONFIGURATION

            MAP_BEGIN=0
            MAP_END=$(grep -e "MAP,.*END" -m 1 $StateTraceFile | cut -d',' -f 4)
            REDUCE_BEGIN=$(grep -e "REDUCE,.*START" -m 1 $StateTraceFile | cut -d',' -f 4)
            REDUCE_END=$(grep -e "REDUCE,.*END" -m 1 $StateTraceFile | cut -d',' -f 4)

            MAP_DURATION=`awk "BEGIN {printf \"%.2f\n\", $MAP_END-$MAP_BEGIN}"`
            REDUCE_DURATION=`awk "BEGIN {printf \"%.2f\n\", $REDUCE_END-$REDUCE_BEGIN}"`
            TOTAL_DURATION=`awk "BEGIN {printf \"%.2f\n\", $REDUCE_END-$MAP_BEGIN}"`
            
            AppendCSVLine $COMPLETION_FILE_NAME $TOPOLOGY $NUMBER_OF_NODES $PLATFORM $BYZANTINE_PERC $MAP_DURATION $REDUCE_DURATION $TOTAL_DURATION "TRUE" $CONFIGURATION


            ## Compute stats for CPU consumption
            echo "... computing CPU consumption"
            
            # Erase old data
            #eraseOldDataInTable $CPU_USAGE_FILE_NAME $TOPOLOGY $NUMBER_OF_NODES $PLATFORM $BYZANTINE_PERC $CONFIGURATION
            
            grep "power_used" "$VariableTraceFile" | while read -r line ; do
                TIME=$(echo $line | cut -d',' -f 6)
                POWER=$(echo $line | cut -d',' -f 7)
                AppendCSVLine $CPU_USAGE_FILE_NAME $TOPOLOGY $NUMBER_OF_NODES $PLATFORM $BYZANTINE_PERC $POWER $TIME "TRUE" $CONFIGURATION                
            done
        fi
    done
    cd .. 
done

rm "$TMP_FILE_CPU" "$TMP_FILE_DURATION"

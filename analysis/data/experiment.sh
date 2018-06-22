#!/bin/bash

VariableTraceFile="Cluster-10/variable.MARS-0.csv"

grep "power_used" "$VariableTraceFile" | while read -r line ; do
    TIME=$(echo $line | cut -d',' -f 6)
    POWER=$(echo $line | cut -d',' -f 7)
    echo $POWER $TIME                
done
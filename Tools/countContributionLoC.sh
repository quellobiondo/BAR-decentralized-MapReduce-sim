#! /bin/bash

STATS=`git diff --numstat 564c01ca8d57bec80169e1492db68dddc1d39eb2 master | grep -E '\.(c|h)$'`
ADDITIONS=`echo "$STATS" | awk '{sum += $1} END {print sum}'`
DELETIONS=`echo "$STATS" | awk '{sum += $2} END {print sum}'`
echo "+ $ADDITIONS"
echo "- $DELETIONS"
 

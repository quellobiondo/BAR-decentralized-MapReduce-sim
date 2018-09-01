#! /bin/bash

echo "Create the required docker container..."

TOOL_FOLDER="$PWD"
ENTRY_POINT_SCRIPT_NAME="entrypoint.sh"
ENTRY_POINT_SCRIPT_ORIGINAL="$PWD/$ENTRY_POINT_SCRIPT_NAME"

SIMGRID_DOCKERFILE="SimGrid.Dockerfile"
MARS_PLATFORM="$PWD/.."
MARS_PESSIMISTIC_PLATFORM="$PWD/../Other platforms/MARS-pessimistic"
HADOOP_PLATFORM="$PWD/../Other platforms/Hadoop"
HADOOP_BFT_PLATFORM="$PWD/../Other platforms/HadoopBFT"
BLOCKCHAIN_PLATFORM="$PWD/../Other platforms/Blockchain"

echo ""
echo "#####"
echo "[SimGrid] building -> $SIMGRID_DOCKERFILE"
echo "#####"
echo ""
docker build -t simgrid:3.11.1 -f SimGrid.Dockerfile .

echo ""
echo "#####"
echo "[MARS] building -> $MARS_PLATFORM"
echo "#####"
echo ""
cd "$MARS_PLATFORM"
docker build --no-cache -t marssim:byzantines .

echo ""
echo "#####"
echo "[HadoopBFT] building -> $HADOOP_BFT_PLATFORM"
echo "#####"
echo ""
cd "$HADOOP_BFT_PLATFORM"
cp "$ENTRY_POINT_SCRIPT_ORIGINAL" "$HADOOP_BFT_PLATFORM/Tools/$ENTRY_POINT_SCRIPT_NAME"
docker build --no-cache -t hadoopbftsim:byzantines .

echo ""
echo "#####"
echo "[Hadoop] building -> $HADOOP_PLATFORM"
echo "#####"
echo ""
cd "$HADOOP_PLATFORM"
cp "$ENTRY_POINT_SCRIPT_ORIGINAL" "$HADOOP_PLATFORM/Tools/$ENTRY_POINT_SCRIPT_NAME"
docker build --no-cache -t hadoopsim .

echo ""
echo "#####"
echo "[Blockchain] building -> $BLOCKCHAIN_PLATFORM"
echo "#####"
echo ""
cd "$BLOCKCHAIN_PLATFORM"
cp "$ENTRY_POINT_SCRIPT_ORIGINAL" "$BLOCKCHAIN_PLATFORM/Tools/$ENTRY_POINT_SCRIPT_NAME"
docker build --no-cache -t blockchainsim .

echo ""
echo "#####"
echo "[MARS Pessimistic] building -> $MARS_PESSIMISTIC_PLATFORM"
echo "#####"
echo ""
cd "$MARS_PESSIMISTIC_PLATFORM"
cp "$ENTRY_POINT_SCRIPT_ORIGINAL" "$MARS_PESSIMISTIC_PLATFORM/Tools/$ENTRY_POINT_SCRIPT_NAME"
docker build --no-cache -t marssim:pessimistic .

cd "$TOOL_FOLDER"
echo "Got back to $TOOL_FOLDER"

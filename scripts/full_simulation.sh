#!/bin/bash

BINARY_DIR=../bin/

$BINARY_DIR/ipc_hub &
$BINARY_DIR/eth_gateway &
$BINARY_DIR/virtual_car -cc 4 -vm cam &
$BINARY_DIR/video_viewer &
#$BINARY_DIR/lane_detection &

# Add any additional process you want to run here.
#$BINARY_DIR/<your_process_name> &

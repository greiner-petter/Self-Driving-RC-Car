#!/bin/bash

BINARY_DIR=../bin/

$BINARY_DIR/ipc_hub &
$BINARY_DIR/can_gateway &
$BINARY_DIR/eth_gateway &
$BINARY_DIR/camera -sh 2 -sv 2 -c 3 -gp 100 -exp 4 -\!as &
$BINARY_DIR/image_processing_bev &
$BINARY_DIR/lane_detection &

# Add any additional process you want to run here.
#$BINARY_DIR/<your_process_name> &

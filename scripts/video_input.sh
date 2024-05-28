#!/bin/bash

BINARY_DIR=../bin/

file=$1
if ! test -f $file; then
  echo "Couldn't find video file!"
  echo "Exiting"
  exit
fi

$BINARY_DIR/ipc_hub &
$BINARY_DIR/eth_gateway &
$BINARY_DIR/video_input $file &
$BINARY_DIR/video_viewer &
$BINARY_DIR/image_processing_bev &

# Add any additional process you want to run here.
#$BINARY_DIR/<your_process_name> &

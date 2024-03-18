#!/bin/bash

BINARY_DIR=../bin/

$BINARY_DIR/ipc_hub &
$BINARY_DIR/eth_gateway &
$BINARY_DIR/virtual_car &

# Add any additional process you want to run here.
#$BINARY_DIR/<your_process_name> &

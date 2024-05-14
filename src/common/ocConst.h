#pragma once

#define OC_SOCKET_PATH "/tmp/occar_ipc_socket"
#define OC_SOCKET_LOCK_PATH "/tmp/occar_ipc_socket.lock"

// magic value that is sent from the ipc hub to a new client
#define OC_AUTH_PASSWORD        0x12345678

// Image properties
#define OC_CAM_BUFFER_SIZE (1024 * 1280 * 4)
#define OC_NUM_CAM_BUFFERS 3
#define OC_BIN_BUFFER_SIZE (1024 * 1280 * 1)
#define OC_NUM_BIN_BUFFERS 2
#define OC_BEV_BUFFER_SIZE (400 * 400 * 1)
#define OC_NUM_BEV_BUFFERS 2

#!/bin/bash

# Kill any process with /bin/ in the name. Not the most ideal way, but it worked so far.
for pid in $(pgrep -f "/bin//") ; do kill -9 $pid ; done

cansend can0 011#00 # set speed to 0
cansend can0 091#00000000 # turn off the lights

exit 0

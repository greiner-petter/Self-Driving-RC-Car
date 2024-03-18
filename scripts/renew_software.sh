#!/bin/bash

path="$(dirname "$0")/.."

cd "$path"

git pull

# The "&& \" at the end of the lines means that the next line
# will only be executed, if the current one was successful.
# So we don't start a build if we couldn't generate the build scripts
# and we don't run the tests if we couldn't build the code.

# Change the commenting on the following lines to switch between
# debug and release build.
cmake --warn-uninitialized -B build -DCMAKE_BUILD_TYPE=Debug && \
#cmake --warn-uninitialized -B build -DCMAKE_BUILD_TYPE=Release && \
cmake --build build -j6 && \
cmake --build build -j6 --target test

echo "Build all car bins and libs."
# Provide that all binaries are written on storage from memory
# In case of powerloss (e.g. remove battery without shutdown EDI)
# all processes should be stored on EDI
sync $path/occar
sync $path/occar
sync $path/occar

# The variable $? is the return code from the previous command. This way
# whoever runs this script can know if the build was successful or not.
exit $?

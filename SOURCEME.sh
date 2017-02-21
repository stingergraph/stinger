#!/bin/bash

echo Run this from your build directory ONLY!
if [ ! -e data ]; then
  ln -s ../util/graph_explorer/ data
fi
export PATH=`pwd`/bin:$PATH
export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
export PYTHONPATH=`pwd`/lib:$PYTHONPATH

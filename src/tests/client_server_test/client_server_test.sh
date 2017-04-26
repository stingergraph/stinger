#!/bin/bash

set -e
export STINGER_ROOT_PATH=$1
export STINGER_USE_TCP=$2
export STINGER_RUN=`pwd`
export STINGER_FILE=$STINGER_RUN/stinger.graph
export STINGER_FILE_TYPE=r
export STINGER_LOGDIR=$STINGER_RUN/log
export PORT_STREAMS=10102
export PORT_ALGS=10103
export STINGER_MAX_MEMSIZE=64M

dumplogs()
{
  for file in $STINGER_LOGDIR/*; do
    echo "Dumping $file..."
    cat $file
  done
  exit 1
}

trap dumplogs ERR

rm -f $STINGER_FILE
rm -rf $STINGER_LOGDIR

./stingerctl start
./stingerctl addalg random_edge_generator -n 1024 -x 100 -d 1
./stingerctl addalg pagerank
sleep 1
./stingerctl addalg clustering_coefficients
sleep 1
./stingerctl addalg betweenness
sleep 1
./stingerctl remalg pagerank
sleep 1
./stingerctl remalg betweenness
sleep 1
./stingerctl remalg clustering_coefficients
sleep 1
./stingerctl remalg random_edge_generator
./stingerctl stop

rm -f $STINGER_FILE
rm -rf $STINGER_LOGDIR

exit 0

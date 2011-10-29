#!/bin/bash

#Get current director;
CURRENT=`pwd`

export GOHOSTOS=linux
export GOHOSTARCH=amd64
export GOOS=ucoresmp
export GOARCH=amd64
export GOROOT=$CURRENT
export PATH=$PATH:$GOROOT/bin

cd $GOROOT/src
. ./make.bash


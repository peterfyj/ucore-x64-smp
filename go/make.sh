#!/bin/bash

#Get current director;
CURRENT=`pwd`

export GOHOSTOS=linux
export GOHOSTARCH=amd64
export GOOS=ucoresmp
export GOARCH=amd64
export GOROOT=$CURRENT
export PATH=$PATH:$GOROOT/bin

build_go()
{
	cd $GOROOT/src
	. ./make.bash
}

diff_go()
{
	original_go="`readlink "$CURRENT/../../go" -f`"
	diff "$original_go/src" "$CURRENT/src" -r -x "Make.deps" -q
}

clean_go()
{
	cd $GOROOT/src
	. ./clean.bash
}

compile_go()
{
	6g $1
}

case $1 in
	clean)
		clean_go
		exit
		;;
	diff)
		diff_go
		exit
		;;
	compile)
		compile_go $2
		exit
		;;
	make | build)
		build_go
		exit
		;;
esac



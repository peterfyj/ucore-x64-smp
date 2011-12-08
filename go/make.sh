#!/bin/bash

#Get current director;
CURRENT=`pwd`

export GOHOSTOS=linux
export GOHOSTARCH=amd64
export GOOS=ucoresmp
export GOARCH=amd64
export GOROOT=$CURRENT
export PATH=$PATH:$GOROOT/bin
export CGO_ENABLED=0

alias _runtime='cd "$CURRENT/src/pkg/runtime/ucoresmp"'
alias _root='cd "$CURRENT"'

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
	cd "$GOROOT/testsuit"
	6g "$1.go" && 6l "$1.6"
	mv 6.out "$GOROOT/../ucore/src/user-ucore/_initial/"
	rm "$1.6"
	rm "$GOROOT/../ucore/obj/sfs.img"
}

rebuild_pkg()
{
	cd "$GOROOT/src/pkg/$1"
	make clean
	make
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
	rebuild)
		rebuild_pkg $2
		exit
		;;
	'')
		;;	
	*)
		echo "Unrecognized parameter."
		exit
		;;
esac



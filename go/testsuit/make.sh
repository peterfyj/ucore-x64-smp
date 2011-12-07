#!/bin/bash

cd ..
. make.sh
cd testsuit
6g hw2.go
6l hw2.6
rm hw2.6
mv 6.out ../../ucore/src/user-ucore/_initial/6.out
rm ../../ucore/obj/sfs.img

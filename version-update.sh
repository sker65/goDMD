#!/bin/bash

version=$(git describe --always --tags | awk -F- '{print $1}')
echo "updating to version $version"
awk -v v=$version '{if( $0 ~ /define VERSION / ) print $1,$2,"\"goDMD v" v "\""; else print $0;}' version.h > version.tmp
mv version.tmp version.h


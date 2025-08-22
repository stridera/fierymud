#!/bin/bash


one=$1
dir1='/opt/PRODUCTION/lib/world/zon'
dir2='/opt/TEST/lib/world/zon'

if [ "${one}" == "" ] 
then
   for file in ${dir1}/*.zon; do
       diff -q "$dir1/${file##*/}" "$dir2/${file##*/}"
   done
else
   diff "${dir1}/${one}" "${dir2}/${one}"
fi


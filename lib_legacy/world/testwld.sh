#!/bin/bash


one=$1
dir1='/obj/PRODUCTION/lib/world/wld'
dir2='/obj/TEST/lib/world/wld'

if [ "${one}" == "" ] 
then
   for file in ${dir1}/*.wld; do
       diff -q "$dir1/${file##*/}" "$dir2/${file##*/}"
   done
else
   diff "${dir1}/${one}" "${dir2}/${one}"
fi


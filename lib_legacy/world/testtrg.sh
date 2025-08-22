#!/bin/bash


one=$1
dir1='/opt/PRODUCTION/lib/world/trg'
dir2='/opt/TEST/lib/world/trg'

if [ "${one}" == "" ] 
then
   for file in ${dir1}/*.trg; do
       diff -q "$dir1/${file##*/}" "$dir2/${file##*/}"
   done
else
   diff "${dir1}/${one}" "${dir2}/${one}"
fi


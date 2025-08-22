#!/bin/bash


one=$1
dir1='/opt/PRODUCTION/lib/world'
dir2='/opt/TEST/lib/world'

if [ "${one}" == "" ] 
then
   echo " this checks all wld, zon, trg, obj, and shp files of a specific zone"
   echo " with it's counterpart in TEST -- you need to supply a zone number"
   echo " "
else
   echo " "
## objs
   echo "OBJs for $one"
   diff -q -s "${dir1}/obj/${one}.obj" "${dir2}/obj/${one}.obj"
   echo ""
## trgs
   echo "TRGs for $one"
   diff -q -s "${dir1}/trg/${one}.trg" "${dir2}/trg/${one}.trg"
   echo ""
## Mobs
   echo "MOBs for $one"
   diff -q -s "${dir1}/mob/${one}.mob" "${dir2}/mob/${one}.mob"
   echo ""

## ZONs
   echo "ZONs for $one"
   diff -q -s "${dir1}/zon/${one}.zon" "${dir2}/zon/${one}.zon"
   echo ""

## wld
   echo "WLDs for $one"
   diff -q -s "${dir1}/wld/${one}.wld" "${dir2}/wld/${one}.wld"
   echo ""

## shps
   echo "SHPs for $one"
   diff -q -s "${dir1}/shp/${one}.shp" "${dir2}/shp/${one}.shp"
   echo ""




fi


#!/bin/bash

if [ $1 == "" ] ; then
	echo ""
	echo "this creates all the appropriate area, mob, trigger, etc files for an area"
	echo "You need to provide the zone #, as an argument "
	echo "example: ./create-area.sh 45"
	echo ""
        exit 1 	
fi

#add a mob file
if [ -f /mob/$1.mob ]; then 
	echo "$1.mob already exists"
else
	echo "$" >> mob/$1.mob
fi

# add trigger file
if [ -f /trg/$1.trg ]; then
        echo "$1.trg already exists"
else
        echo "$~" >> trg/$1.trg
fi

# create zone file
if [ -f /zon/$1.zon ]; then
        echo "$1.zon already exists"
else
	echo "#$1" >> zon/$1.zon
	echo "unnamed zone" >> zon/$1.zon
	echo "$105 30 2 100 0 0" >> zon/$1.zon
	echo "S" >> zon/$1.zon
        echo "$" >> zon/$1.zon
fi

#add a wld file
if [ -f /wld/$1.wld ]; then
        echo "$1.wld already exists"
else
        echo "$" >> wld/$1.wld
	echo "#$100" >> wld/$1.wld
	echo "The Beginning~" >> wld/$1.wld
	echo "Not much here." >> wld/$1.wld
	echo "~" >> wld/$1.wld
	echo "64 0 0" >> wld/$1.wld
	echo "S" >> wld/$1.wld
	echo "$" >> wld/$1.wld
fi

# add shop file
if [ -f /shp/$1.shp ]; then
        echo "$1.shp already exists"
else
        echo "$~" >> shp/$1.shp
fi

# add obj file
if [ -f /obj/$1.obj ]; then
        echo "$1.obj already exists"
else
        echo "$~" >> obj/$1.obj
fi


#adding to index files.
types=("mob" "trg" "zon" "wld" "shp" "obj") 
for typ in ${types[@]}; do
     echo Adding to $typ index
     if grep -Fq $1.$typ $typ/index; then
          echo $1.$typ already exists in $typ/index
     else
          sed -i 's/\$//g' $typ/index
	  sed -i '/^$/d' $typ/index
          echo "$1.$typ" >> $typ/index
          sort -h -o $typ/index $typ/index
	  echo "" >> $typ/index
          echo "$" >> $typ/index
     fi
done
# mv -f $file.new $file




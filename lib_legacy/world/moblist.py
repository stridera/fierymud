import os

mobpath = '/mud/PRODUCTION/mud/lib/world/mob/'
mobfile = 'moblist.txt'

filelist = []

# build files list
for root, directories, files in os.walk(mobpath, topdown=False):
     for name in files:
         if (name[-4:] == ".mob"):
              filelist.append(os.path.join(root, name))
filelist.sort()
#print (filelist)         
         

#
objWrite = open(mobfile,"w")

for fname in filelist:
     objread = open(fname,"r")
     line = objread.readline() 
     while line:
          #print(line)
          line=objread.readline()
          if (line[:1] == "#"):
               line = line.rstrip('\r\n')
               wrilin = (line[1:len(line)])
               tmp=objread.readline()
               line=objread.readline()
               line = line.rstrip('\r\n')
               wrilin += ", "
               wrilin += line[:-1]
               wrilin += "\n"
               #print (wrilin)
               objWrite.writelines(wrilin)
     objread.close()
          
          
objWrite.close()

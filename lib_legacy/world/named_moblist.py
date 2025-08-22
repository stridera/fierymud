import os

mobpath = '/mud/PRODUCTION/mud/lib/world/mob/'
mobfile = 'moblist_named.txt'
# we convert all matches to lower case, so keep the below in lower case too
genericnames = ['a', 'an', 'the']

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
               # lets only write NAMED mobs 
               nmob = wrilin.split()
               #print(nmob[1])
               flag=0
               for match in genericnames:
                    if nmob[1].lower() == match:
                        flag =1;
               if flag == 0:
                    objWrite.writelines(wrilin)
     objread.close()
          
          
objWrite.close()

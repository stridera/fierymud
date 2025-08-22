import os,glob,sys
#look at args first.. should be 2
n = len(sys.argv)
#print ("arguments")
#print (n)
if (n != 3):
    quit('arguments incorrect -- vnum first, lines after next')
rng = int(sys.argv[2]) 
mpath = '/opt/PRODUCTION/lib/players/'
dirlist = os.listdir(mpath)
for i in dirlist:
    if os.path.isdir(i):
        cpath = mpath + i + "/*.objs"
        clist = glob.glob(cpath)
        for cfile in clist:
            file = open(cfile,'r')
            ct = 0
            found = 0
            lines = file.readlines()
            for line in lines:
                vnum = "vnum: "+sys.argv[1]
                ct+=1
                if vnum in line:
                    print (line), 
                    found = ct
                if (ct > found) & (ct == (found+rng)):
                    print (line)
            file.close()
             

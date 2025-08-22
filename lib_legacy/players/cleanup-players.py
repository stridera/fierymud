import os,glob,sys
#look at args first.. should be 2
n = len(sys.argv)
# print ("arguments")
# print (n)
if (n != 3):
    quit('arguments incorrect -- level , Months of inactivity')
# rng = int(sys.argv[2]) 
mpath = '/opt/PRODUCTION/lib/players/'
dirlist = os.listdir(mpath)
for i in dirlist:
    if os.path.isdir(i):
        cpath = mpath + i + "/*.plr"
        clist = glob.glob(cpath)
        for cfile in clist:
            file = open(cfile,'r')
            ct = 0
            lev = 0
            last = 0
            lines = file.readlines()
            for line in lines:
                lev = "level: "
                ct+=1
                if lev in line:
                    level = lev[6:] 
                    print (level) 
                    print (line)
            file.close()
             

## arguments 
## 1) file path to traverse
## 2) new directory

import os
import sys
import subprocess

rootPath = sys.argv[1]

os.system( 'mkdir '+sys.argv[2] )

for root, dirs, files in os.walk( rootPath ):
        for file in files:
            fullPath = os.path.join( root, file )
            if not ".svn" in fullPath:
                ##print( fullPath )
                command = 'cp \"' + fullPath + '\" \"' + sys.argv[2] + '/' + file + '\"'
                ##print command
                os.system( command )
	
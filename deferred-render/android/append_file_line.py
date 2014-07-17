import sys
import os

srcFile = sys.argv[1]
type = sys.argv[2]

file = open( srcFile, 'r' )
lines = file.readlines()
file.close()

newFile = open( sys.argv[1], 'w' )
for line in lines:
    if line.find( 'RELEASE 1' ) == -1 and line.find( 'DEBUG 1' ) == -1:
        newFile.write( line )
    
if type == 'debug':
    newFile.write( '#define DEBUG 1' )
elif type == 'release':
    newFile.write( '#define RELEASE 1' )    
    
newFile.close()
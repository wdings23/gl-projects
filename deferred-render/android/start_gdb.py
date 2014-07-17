import subprocess
import os
import time

#
#
#
def killPrevGDB():
    p = subprocess.Popen( 'adb shell ps | grep gdb',
                         shell = True,
                         stdout = subprocess.PIPE,
                         stderr = subprocess.STDOUT )
                         
    pid = None
    for line in p.stdout.readlines():
        words = line.split( ' ' )
        for word in words:
            if word.isdigit():
                pid = word
                break

    if pid != None:
        print 'PID to kill = ' + pid
        p = subprocess.Popen( 'adb shell su -c \'kill -9 ' + pid + '\'',
                             shell = True,
                             stdout = subprocess.PIPE,
                             stderr = subprocess.STDOUT )

    for line in p.stdout.readlines():
        print line

    print 'killed previous gdb session'

#
#
#
def launchApp():
    p = subprocess.Popen( 'adb shell am start -n com.tableflipstudios.worldviewer/.GameActivity',
                          shell = True,
                          stdout = subprocess.PIPE,
                          stderr = subprocess.STDOUT )
                          
    print 'launched app'

#
#
#
def getPID():
    # need to wait for the app to start
    time.sleep( 10 )
    
    
    p = subprocess.Popen( 'adb shell ps | grep worldviewer',
                          shell = True,
                          stdout = subprocess.PIPE,
                          stderr = subprocess.STDOUT )
    
    pid = 'none'
    for line in p.stdout.readlines():
        words = line.split( ' ' )
        for word in words:
            print 'word = ' + word
            if word.isdigit():
                pid = word
                break

    retval = p.wait()
    print 'pid = ' + pid

    return pid

#
#
#
def startGDBServer( pid ):

    # forward port commands to 5039
    p = subprocess.Popen( 'adb forward tcp:5039 tcp:5039',
                         shell = True,
                         stdout = subprocess.PIPE,
                         stderr = subprocess.STDOUT )
    
    
    print 'attach gdb server to ' + pid
    p = subprocess.Popen( 'adb shell su -c \'/data/data/com.tableflipstudios.worldviewer/lib/gdbserver :5039 --attach ' + pid + ' &\'',
                          shell = True,
                          stdout = subprocess.PIPE,
                          stderr = subprocess.STDOUT )

    for line in p.stdout.readlines():
        print line

    print 'gdbserver attached'

def pullAppProcess():
    
    p = subprocess.Popen( 'adb pull /system/bin/app_process',
                         shell = True,
                         stdout = subprocess.PIPE,
                         stderr = subprocess.STDOUT )
                         
    for line in p.stdout.readlines():
        print line

    print 'pulled app_process'

def startGDB():
    os.system( '/Users/dingwings/Downloads/android-ndk-r9b/toolchains/arm-linux-androideabi-4.6/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gdb -tui -x gdb.setup app_process' )
    print 'started gdb'

killPrevGDB()
launchApp()
pid = getPID()
startGDBServer( pid )
pullAppProcess()
startGDB()

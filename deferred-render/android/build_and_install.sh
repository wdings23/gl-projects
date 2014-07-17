set -e

python copyassets.py ../assets ./assets

cd jni
ndk-build NDK_DEBUG=1 APP_OPTIM=debug -j4 #V=1
cd ..
ant debug

adb uninstall com.tableflipstudios.worldviewer
adb install ./bin/GameActivity-debug.apk

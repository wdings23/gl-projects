set -e

rm -rf bin
rm -rf libs
rm -rf obj

python copyassets.py ../assets ./assets
python append_file_line.py ./jni/Prefix.h release

cd jni
ndk-build NDK_DEBUG=0 APP_OPTIM=release -j4 #V=1
cd ..
ant release

jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ./needsnacks-release-key.keystore ./bin/graffcity-release-unsigned.apk needsnacks
jarsigner -verify ./bin/graffcity-release-unsigned.apk
zipalign -v 4 ./bin/graffcity-release-unsigned.apk ./bin/graffcity.apk

adb uninstall com.needsnacks.graffcity
adb install ./bin/graffcity.apk

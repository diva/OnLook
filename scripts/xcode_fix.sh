#!/bin/sh

echo 'Linden fix for Xcode 4.6 to have 10.6SDK so it can build older branches.'

echo 'Be sure to have the xcode_4.3_for_lion.dmg mounted! This script pulls from that volume!'

echo 'Creating temporary directory. . .'

mkdir temp

pushd temp

echo 'Copying 10.6SDK. . .'

cp -R /Volumes/Xcode/Xcode.app/Contents//Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.6.sdk .

echo 'Linking darwin10 folders as darwin11. . .'

pushd MacOSX10.6.sdk/Developer/usr/llvm-gcc-4.2/lib/gcc/

ln -s i686-apple-darwin10 i686-apple-darwin11

popd

pushd MacOSX10.6.sdk/usr/lib/gcc/

ln -s i686-apple-darwin10 i686-apple-darwin11

popd

pushd MacOSX10.6.sdk/usr/lib/

ln -s i686-apple-darwin10 i686-apple-darwin11

popd

echo 'Changing ownership and moving SDK to machine. . . Password required for sudo commands.'

sudo chown -R root:wheel MacOSX10.6.sdk

sudo mv MacOSX10.6.sdk/ /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs

popd

rm -rf temp

echo 'Xcode fix complete.'


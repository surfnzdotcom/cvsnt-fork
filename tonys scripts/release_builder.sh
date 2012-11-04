#!/bin/sh
(shopt -s igncr) 2>/dev/null && shopt -s igncr; # cygwin bug workaround
BRANCH=CVSNT_2_0_x
PROGFILES="Program Files"
PATH=/cygdrive/d/cvsbin:${PATH}:/cygdrive/c/"$PROGFILES"/winzip:/cygdrive/c/"$PROGFILES"/winzip:/cygdrive/c/"$PROGFILES"/PuTTY:/cygdrive/c/"$PROGFILES"/PuTTY:/cygdrive/c/"$PROGFILES"/"HTML Help Workshop":/cygdrive/c/"$PROGFILES"/"HTML Help Workshop"
rm -rf cvsnt
TOP="`pwd`"
cvs -d :pserver:tmh@cvs.cvsnt.org:/usr/local/cvs co -r $BRANCH cvsnt
cd cvsnt
export CL=/DCOMMERCIAL_RELEASE
devenv.com /rebuild release cvsnt.sln
cd "$TOP"
rm -f ../*
if [ -f ../cvsapi.dll ]; then
  echo Cannot build due to locked files!
  read
  exit 1
fi
export TARGET=/cygdrive/d/cvsbin
export TARGET_WIN32=d:/cvsbin
export BASE="`pwd`/cvsnt"
export TYPE=winrel
sh cvsnt/tonys\ scripts/copy_common.sh
pushd "$TARGET_WIN32"
for i in *.exe *.dll *.cpl database/*.dll mdns/*.dll protocols/*.dll triggers/*.dll xdiff/*.dll
do
  echo signcode /n "March Hare Software Ltd" /t http://timestamp.verisign.com/scripts/timestamp.dll "$i"
  signcode /n "March Hare Software Ltd" /t http://timestamp.verisign.com/scripts/timestamp.dll "$i"
done
popd

cd cvsnt
BUILD=`$TARGET/cvs.exe ver -q`
echo Build=$BUILD
echo $BUILD >doc/version.inc
TAG=CVSNT_`echo $BUILD | sed 's/\./_/g'`
echo Tag=$TAG
perl.exe /cvsdeps/cvs2cl/cvs2cl.pl -F $BRANCH --window 3600
$TARGET/cvs.exe -q commit -m "Build $BUILD"
$TARGET/cvs.exe tag -F $TAG
cd "$TOP"

cd cvsnt/doc
./build.bat cvs
./build-pdk.bat $BUILD
cd "$TOP"

cp cvsnt/doc/*.chm ..
cp cvsnt/WinRel/*.pdb ../pdb
cp cvsnt/WinRel/protocols/*.pdb ../pdb/protocols
cp cvsnt/WinRel/triggers/*.pdb ../pdb/triggers
cp cvsnt/WinRel/xdiff/*.pdb ../pdb/xdiff

cd /cygdrive/d/march-hare/build
./build.bat
cd "$TOP"

cd cvsnt/installer
cmd.exe /c nmake all
mv cvsnt-client-${BUILD}.msi ../../..
mv cvsnt-server-${BUILD}.msi ../../..
mv suite-client-trial-${BUILD}.msi ../../..
mv suite-server-trial-${BUILD}.msi ../../..
cd ../../..
wzzip -P cvsnt-${BUILD}-bin.zip *.dll *.exe *.cpl *.ini ca.pem infolib.h COPYING triggers protocols xdiff
wzzip -pr cvsnt-${BUILD}-pdb.zip pdb
wzzip -Pr cvsnt-${BUILD}-dev.zip cvsnt-pdk.chm inc lib
echo ${BUILD} >release
pscp -i d:/tony.ppk *.zip *.msi cvs.chm release tmh@obrien:/home/tmh/cvs

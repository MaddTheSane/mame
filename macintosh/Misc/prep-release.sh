#!/bin/bash

#
# A simple shell script to pull a release together. Intended to run from its
# default location in the repository.
#
# Invoke with "./prep-release.sh 0.102u5" to create, e.g., MacMAME-0.102u5.dmg and macsrc0.102u5.dmg
#

#
# Wipe our Distro directory and pull a clean copy from svn into it.
#

DISTRO="../../../../Distro"

echo "Removing old Distro..."
rm -rf $DISTRO

echo "Pulling latest MacMAME code/binaries from svn server..."
#svn export "http://www.brad-oliver.com:6502/svn/MacMAME" $DISTRO
svn export ../../../ $DISTRO

echo "Extracting webloc file..."
ditto -x -k --sequesterRsrc MsgBoardURL.zip $DISTRO/MacMAME

#
# Copy MacMAME.app
#

echo "Copying MacMAME.app..."
cp -R ../../../MacMAME/MacMAME.app $DISTRO/MacMAME/

#
# Set the label color of the Read Me to red.
#

cd $DISTRO/MacMAME

echo "Setting label color for read me..."
OUR_FILE=$PWD"/Read Me First! - No, Really!.rtf"
/usr/bin/osascript <<END
tell application "Finder"
  set pFile to POSIX file "$OUR_FILE" as file
  set label index of item pFile to 2
end tell
END

cd ..

#
# Build the MacMAME disk image
#

echo "Creating MacMAME dmg..."
hdiutil create -imagekey zlib-level=9 -format UDZO -srcfolder "MacMAME" MacMAME-$1.dmg

#
# Build the MacMAME source disk image
#

echo "Creating MacMAME source dmg..."
hdiutil create -imagekey zlib-level=9 -format UDZO -srcfolder "MacMAME Source" macsrc$1.dmg

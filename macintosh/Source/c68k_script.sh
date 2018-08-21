#!/bin/bash

#
# A simple shell script to kick-start the C 68k core build process.
#

# Move to the C 68k source directory

echo "Changing dir to ${MW_OUTPUT_DIRECTORY}"
cd "${MW_OUTPUT_DIRECTORY}"
echo pwd
pwd

# Run the command-line app, which will generate the source files.
# Make sure the source is in Unix line-ending format, or m68make will
# spew out some odd EOF warnings.

./m68kmake
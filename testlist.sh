#!/bin/bash

# Set up
export TMPDIR=$(pwd)/tmp
mkdir -p $TMPDIR
./ts -K

# Test
./ts echo "Short command"
./ts -L "With label" echo "Short command"
./ts -L "Just a short nap..." sleep 1
./ts -L "Very long sleep... ZzZzZz..." sleep 2
./ts -L "After long sleep, we must echo stuff" echo "Much stuff and even more stuff!"
./ts echo "Looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooongest echo ever!"
./ts echo "This line is barely fitting into the extra command line if table_width == 100. Close call!"
./ts echo "On the other hand, again with table_width == 100, this line is just one character too long!"
./ts echo "Barely fitting on thesameline!"

echo "Before:"
./ts -l
echo "Waiting 4 seconds..."
sleep 4
echo "After:"
./ts -l

# Clean up
./ts -K
rm -rf $TMPDIR

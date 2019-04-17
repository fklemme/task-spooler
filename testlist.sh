#!/bin/bash

# Set up
export TMPDIR=$(pwd)/tmp
mkdir -p $TMPDIR
./ts -K

# Test
./ts echo "Short command" # job 0
./ts "echo 'failing...' && false" # job 1
./ts -L "Just a short nap..." sleep 1 # job 2
./ts -L "Very long sleep... ZzZzZz..." sleep 2 # job 3
./ts -L "After long sleep, we must echo stuff" echo "Much stuff and even more stuff!" # job 4
./ts echo "Loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooongest echo ever!" # job 5
./ts echo "This line is barely fitting into the extra command line if table_width == 100. Close call!" # job 6
./ts echo "On the other hand, again with table_width == 100, this line is just one character too long!" # job 7
./ts echo "Barely fitting on thesameline!" # job 8
./ts -D 3 -L "Just testing how dependencies are displayed" echo "Such a long label!" # job 9
./ts -D 1 echo "Short command fits line!" # job 10

echo "Right after setup:"
./ts -l
echo "Waiting 4 seconds..."
sleep 4
echo "When all finished:"
./ts -l

# Clean up
./ts -K
rm -rf $TMPDIR

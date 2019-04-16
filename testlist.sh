#!/bin/bash

./ts -K

./ts echo "Short command"
./ts -L "With label" echo "Short command"
./ts sleep 2
./ts -L "Very long sleep... ZzZzZz..." sleep 5
./ts -L "After long sleep, we must echo stuff" echo "Much stuff and even more stuff!"
./ts echo "Looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooongest echo ever!"

echo "Before:"
./ts -l
echo "Waiting..."
sleep 10
echo "After:"
./ts -l

./ts -K

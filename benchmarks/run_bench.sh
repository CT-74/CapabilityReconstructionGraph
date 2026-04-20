#!/bin/bash
FILE=$1 # On passe le nom sans extension (ex: crg_benchmark_bar)

echo "--- Compiling $FILE.cpp ---"
g++ -O3 -std=c++17 $FILE.cpp -o $FILE.bin

if [ $? -eq 0 ]; then
    echo "--- Running Benchmark ---"
    ./$FILE.bin > $FILE.csv
    
    echo "--- Generating Graph with $FILE.py ---"
    python3 $FILE.py
    echo "Done! Check your .png files."
else
    echo "Compilation failed."
fi
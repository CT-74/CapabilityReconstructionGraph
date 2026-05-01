#!/bin/bash
# Usage: ./compile_and_run.sh crg_benchmark_bar

FILE=$1 

if [ -z "$FILE" ]; then
    echo "Usage: ./compile_and_run.sh <filename_without_extension>"
    exit 1
fi

# Ensure subdirectories exist
mkdir -p bin data

echo "--- Compiling $FILE.cpp to bin/ ---"
clang++ -O3 -march=native -std=c++17 "$FILE.cpp" -o "bin/$FILE.bin"

if [ $? -eq 0 ]; then
    echo "--- Running Benchmark (Output to data/) ---"
    # Execute and redirect output to the data folder
    "./bin/$FILE.bin" > "data/$FILE.csv"
    
    echo "--- Generating Graph with $FILE.py ---"
    python3 "$FILE.py"
    echo "Done! The image has been updated in ../img/"
else
    echo "❌ Compilation failed."
    exit 1
fi
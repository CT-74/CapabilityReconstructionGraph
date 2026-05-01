#!/bin/bash
# Usage: ./run_single.sh script_name.py

SCRIPT_NAME=$1

# 1. Check argument
if [ -z "$SCRIPT_NAME" ]; then
    echo "Error: You must specify the script to run."
    echo "Usage: ./run_single.sh crg_benchmark_architect.py"
    exit 1
fi

# 2. Activate Virtual Environment (if it exists)
if [ -d "venv" ]; then
    echo "Activating virtual environment..."
    source venv/bin/activate
else
    echo "Warning: No local 'venv' found. Using system Python."
fi

# 3. Execute the script
echo "Running $SCRIPT_NAME..."
echo "----------------------------------------"
python3 "$SCRIPT_NAME"
echo "----------------------------------------"

# 4. Cleanup
if [ -d "venv" ]; then
    deactivate
fi

echo "Done!"
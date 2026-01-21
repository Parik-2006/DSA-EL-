#!/usr/bin/env bash
# Exit on error
set -o errexit

echo "Building the project..."

# 1. Install Python dependencies
pip install -r requirements.txt

# 2. Compile the C Backend
# We verify gcc is installed (it usually is) and compile
echo "Compiling C Backend..."
gcc -o backend backend.c

# 3. Make sure it's executable
chmod +x backend

echo "Build successful."
#!/usr/bin/env bash
# Exit on error
set -o errexit

# 1. Install Python dependencies
pip install -r requirements.txt

# 2. Compile the C backend for Linux
gcc backend.c -o backend

# 3. Give execution permission
chmod +x backend
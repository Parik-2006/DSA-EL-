#!/usr/bin/env bash

# 1. Clean up old command files from previous runs
rm -f cmd_*.txt

# 2. Start the C Backend in the background
# The '&' symbol tells Linux to run this parallel to the web server
./backend &

# 3. Start the Web Server (using Gunicorn for production)
gunicorn app:app
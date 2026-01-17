#!/usr/bin/env bash

# 1. FORCE DELETE any existing data files from previous runs or Git
echo "Cleaning up old logs..."
rm -f logs.json
rm -f logs_defense.json
rm -f trie_view.json
rm -f cmd_*.txt

# 2. Start the C Backend in the background
# We add './' to be safe and '&' to run it alongside Python
echo "Starting Backend..."
./backend &

# 3. Start the Web Server
echo "Starting Web Server..."
gunicorn app:app
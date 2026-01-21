#!/usr/bin/env bash

# 1. Run the C Backend in the background (& symbol does this)
# We hide output to keep logs clean, or keep it for debugging
./backend &

# 2. Sleep for 1 second to ensure C engine is ready
sleep 1

# 3. Start the Python Web Server using Gunicorn
# 'app:app' means look in 'app.py' for the 'app' object
gunicorn app:app
#!/usr/bin/env bash

# Start the Python Web Server using Gunicorn
# 'app:app' means look in 'app.py' for the 'app' object
# Bind to 0.0.0.0 and use the PORT environment variable
gunicorn --bind 0.0.0.0:${PORT:-5000} app:app
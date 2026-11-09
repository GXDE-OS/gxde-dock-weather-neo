#!/bin/bash
# Refresh the .ts files from all tr() calls in the project sources.

cd "$(dirname "$0")/.." || exit 1

# The .ts files are taken from TRANSLATIONS in weather.pro.
lupdate weather.pro -no-obsolete

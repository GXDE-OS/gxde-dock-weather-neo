#!/bin/bash
# Generate .qm files from the .ts files for manual testing.
# A normal build does this automatically (see weather.pro).

cd "$(dirname "$0")" || exit 1

for ts in ./*.ts; do
    [ -e "$ts" ] || continue
    lrelease "$ts" -qm "${ts%.ts}.qm"
done

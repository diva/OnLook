#!/bin/bash
#clean up files left over from a patching session
#only run right before you commit!

find . -name "*.orig" | xargs rm
find . -name "*.rej" | xargs rm

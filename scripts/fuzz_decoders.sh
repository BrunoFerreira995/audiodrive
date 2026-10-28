#!/bin/bash
# Fuzz file decoders with random inputs
for i in 1 2 3 4 5; do
    dd if=/dev/urandom of=/tmp/fuzz$i.wav bs=1024 count=10 2>/dev/null
    dd if=/dev/urandom of=/tmp/fuzz$i.mp3 bs=1024 count=10 2>/dev/null
    dd if=/dev/urandom of=/tmp/fuzz$i.flac bs=1024 count=10 2>/dev/null
done
echo "Generated fuzz inputs"

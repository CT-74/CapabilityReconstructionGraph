#!/bin/bash

echo "Creating EP paper project structure..."

mkdir -p paper
mkdir -p pitch
mkdir -p scripts
mkdir -p build/exports

touch paper/paper.md
touch paper/CHANGELOG.md
touch pitch/cppcon_pitch.txt

echo "Done."
echo "Project initialized."

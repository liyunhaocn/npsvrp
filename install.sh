#!/bin/bash
pip install -r requirements.txt
cd dev
make clean
make all
make clean
cd ..

cd baselines/hgs_vrptw
make clean
make all
cd ../..
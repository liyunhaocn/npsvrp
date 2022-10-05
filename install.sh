#!/bin/bash
pip install -r requirements.txt
cd dev
make clean
make all
make clean
cd ..
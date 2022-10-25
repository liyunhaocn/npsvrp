#!/bin/bash
DATE=`date "+%Y-%m-%d-%H-%M-%S"`
mkdir -p submissions
cd submissions
rm -rf tmp
mkdir -p tmp/src/hgs
mkdir -p tmp/src/smart
mkdir -p tmp/dev

mkdir -p tmp/baselines/hgs_vrptw
cp -r ../baselines/hgs_vrptw/{*.cpp,*.h,Makefile} tmp/baselines/hgs_vrptw
cp -r ../baselines/strategies tmp/baselines
cp ../{area_tool.py,delta.py,environment_virtual.py,or_tools.py,solver.py,tools.py,environment.py,run.sh,install.sh,requirements.txt,metadata} tmp

cp -r ../src/{*.cpp,*.h} tmp/src/
cp -r ../src/hgs/{*.cpp,*.h} tmp/src/hgs
cp -r ../src/smart/{*.cpp,*.h} tmp/src/smart
cp -r ../dev/Makefile tmp/dev/Makefile

cd tmp
zip -o -r --exclude=*.git* --exclude=*__pycache__* --exclude=*.DS_Store* ../submission_$DATE.zip .;
cd ../..
echo "Created submissions/submission_$DATE.zip"

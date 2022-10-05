#!/bin/bash
DATE=`date "+%Y-%m-%d-%H-%M-%S"`
mkdir -p submissions
cd submissions
rm -rf tmp
mkdir -p tmp/src/hgs
mkdir -p tmp/src/smart
mkdir -p tmp/dev

cp -r ../src/{*.cpp,*.h,Makefile} tmp/src/
cp -r ../src/hgs/{*.cpp,*.h,Makefile} tmp/src/hgs
cp -r ../src/smart/{*.cpp,*.h,Makefile} tmp/src/smart
cp -r ../dev/Makefile tmp/dev/Makefile

cp ../{strategies.py,solver.py,tools.py,environment.py,run.sh,install.sh,requirements.txt,metadata} tmp
cd tmp
zip -o -r --exclude=*.git* --exclude=*__pycache__* --exclude=*.DS_Store* ../submission_$DATE.zip .;
cd ../..
echo "Created submissions/submission_$DATE.zip"
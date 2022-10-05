#!/bin/bash
# First create a fresh tmp folder in submissions
./create_submission.sh
# Test the submission to see if all dependencies where packed correctly
cd submissions/tmp
./install.sh && python ../../controller.py --instance ../../instances/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt --epoch_tlim 5 -- ./run.sh
cd ../..
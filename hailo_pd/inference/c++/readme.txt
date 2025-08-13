## Install necessary hailo libraries and virtual environment(optional)

cd setup_env
./install.sh
source setup_env.sh

## install necessary libraries

sudo apt-get update && sudo apt-get install -y cmake build-essential libfftw3-dev


## Build and run the project
mkdir build
cd build
cmake ..
make


## run 
./hailo_speech_inference ../speech_model_a.hef ../../audio.wav 


  
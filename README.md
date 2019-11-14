# Step Azure Kinect Demo
Created by: Amon Millner, Richard Gao, Hwei-Shin Harriman for Vision.  
Features implemented:  
* Stick figure dummy bodies, 1 per person
* Stomp detection
* Clap detection
* Emotion classification using Azure Kinect and Azure Cognitive Services
* Sound FX capabilities, can be played upon registering a stomp or a clap
* Visual representation of emotion (classification affects background color)

## Installation Instructions
(Assumes Windows OS and Azure Kinect SDK has been set up according to the [Azure Kinect Documentation](https://docs.microsoft.com/en-us/azure/kinect-dk/set-up-azure-kinect-dk).)  
* Install OpenFrameworks by following instructions at the [OpenFrameworks Website](https://openframeworks.cc/setup/vs/)
* Install the [Azure Kinect Sensor SDK](https://docs.microsoft.com/en-us/azure/Kinect-dk/sensor-sdk-download).
* Install the [Azure Kinect Body Tracking SDK](https://docs.microsoft.com/en-us/azure/Kinect-dk/body-sdk-download).
* Install the Open Frameworks Kinect addon according to [this](https://github.com/prisonerjohn/ofxAzureKinect) Github repository.
* Make sure to copy the cuDNN model file `dnn_model_2_0.onnx` from the Body SDK `tools` folder into your project's `bin` folder!
* Clone this repository into the `apps/myApps` folder within Open Frameworks.
* Install the [cpprestsdk](https://github.com/microsoft/cpprestsdk). This can be done easily with the NuGet Package manager.
* Install OpenCV for C++ and link it with Visual Studio. [This](https://www.deciphertechnic.com/install-opencv-with-visual-studio/) is a helpful tutorial.
* Copy the content of this repo's ofxAzureKinect folder into the src folder of the ofxAzureKinect addon. This should be in `openFrameworks/addons/ofxAzureKinect/src` by default.
* You can then open the `.sln` file in Visual Studio and build. 

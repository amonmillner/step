# Step Azure Kinect Demo
Created by: Amon Millner, Richard Gao, Hwei-Shin Harriman for Vision.  
Features implemented:  
* Stick figure dummy bodies, 1 per person
* Stomp detection
* Clap detection
* Emotion classification using Azure Kinect
* Sound FX capabilities, can be played upon registering a stomp or a clap
* Visual representation of emotion (classification affects background color)

## Installation Instructions
(Assumes Windows OS and Azure Kinect SDK has been set up according to the [Azure Kinect Documentation](https://docs.microsoft.com/en-us/azure/kinect-dk/set-up-azure-kinect-dk).)  
* Install OpenFrameworks by following instructions at the [OpenFrameworks Website](https://openframeworks.cc/setup/vs/)
* Install the [Azure Kinect Sensor SDK](https://docs.microsoft.com/en-us/azure/Kinect-dk/sensor-sdk-download).
* Install the [Azure Kinect Body Tracking SDK](https://docs.microsoft.com/en-us/azure/Kinect-dk/body-sdk-download).
* Install the Open Frameworks Kinect addon according to [this](https://github.com/prisonerjohn/ofxAzureKinect) Github repository.
* Make sure to copy the cuDNN model file `dnn_model_2_0.onnx` from the Body SDK `tools` folder into your project's `bin` folder!
* Clone this repository into the `addons/ofxAzureKinect` folder within Open Frameworks.
* TODO instructions for Azure Kinect Setup.
* You can then open the `.sln` file in Visual Studio and build. 

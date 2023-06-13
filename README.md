# Robot-Xbox-Wireless-Controller
Fun project: Program to control a mobile 4-wheel robot wirelessly using an Xbox controller.

## Compatibility
This program contains two parts: the UDP client script and the Embedded System firmware.
The Firmware is written in C++ for the Particle Photon 1, making it compatible with other particle devices or Arduino boards should be a matter of changing a few constants, mainly the pins used for the motor drivers, but I have not tested this.

The robot that is being controlled consist of a 4-weeled robot with 2 DC motors, one on each side. The embedded system interfaces with the motors through a TB6612 H-Bridge motor driver. Proper wiring of the motor driver is required for the firmware to work.

The UDP client is written in Python3 and should easily run on any platform that supports Python3.

## How to Run
### Embedded System
The firmware can be compiled and flashed to the Particle Photon 1 by clicking the `Compile` button in the Particle IDE. The firmware will automatically connect to the WiFi network and start listening for UDP packets.

### UDP Client
The UDP client is a simple script that sends the state of the Xbox controller to the robot. It is written in Python3 and requires the `inputs` library to be installed. This can be done by running `pip3 install inputs` in the terminal.

The script can be run by simply running `python3 main.py` in the terminal. The script will automatically detect the Xbox controller and connect to it.

The controller should already be paired with the computer before running the script. The script will then prompt the user to enter the IP address of the robot. The IP address can be found by running `particle serial monitor` in the terminal and looking for the line that says `Local IP: xxx.xxx.xxx.xxx`.
In practice, the device will not have a serial connection to the computer, so the IP address is also published to the Particle Cloud. The IP address can be found by logging into the Particle Console and looking for published events from the device.

### How it Works
The UDP client sends the state of the Xbox controller to the robot as a plain text. The robot parses the string and extracts the state of the joysticks and buttons. The robot then uses the state of the joysticks to calculate the speed of each motor and the state of the buttons to determine whether to enable or disable the motors.

#### The System Supports 2 Control Modes
1. Differential Control: the left motor's intensity is controlled by the left trigger and the right motor's intensity is controlled by the right trigger. If both triggers are pressed at the same intensity, the robot will move forward. If the left trigger is pressed more than the right trigger, the robot will turn left. If the right trigger is pressed more than the left trigger, the robot will turn right.

2. Arcade/Racing Control: this is control scheme comes more naturally to individual accustomed to video games. The right trigger is used to move forward, the left trigger is used to move backward, and the left joystick is used to steer left or right.

## Author
Ayoub Mabkhout
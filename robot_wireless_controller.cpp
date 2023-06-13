#include <string.h>

// Define control pins for the motors and debugging peripherals
#define PWMA D1   // PWM pin for controlling the speed of the left motor
#define PWMB D2   // PWM pin for controlling the speed of the right motor
#define IN1A D3   // Control pin for the direction of the left motor
#define IN2A D4   // Control pin for the direction of the left motor
#define IN1B D5   // Control pin for the direction of the right motor
#define IN2B D6   // Control pin for the direction of the right motor
#define LED1 D0   // LED for debugging (e.g., indicating when the system is turning left)
#define BUZZER A4 // Buzzer for debugging (e.g., indicating when the system is turning right)

#define LISTENING_PORT 8888 // The port number to listen for UDP packets

// Global variables for the system state
IPAddress allowedIP(0, 0, 0, 0);    // IP address for filtering incoming UDP packets
UDP udp;                            // UDP object for network communication
SerialLogHandler logHandler(57600); // Serial log handler for debug output
int mode = 0;                       // Mode variable to switch between two driving modes
long last_switched = millis();      // Timestamp of the last mode switch

// Structure to hold the inputs from the Xbox controller
typedef struct
{
    float ljx; // Left joystick x-axis
    float ljy; // Left joystick y-axis
    int a;     // A button
    float lt;  // Left trigger
    float rt;  // Right trigger
} XboxInput;

// Function to parse the input message from the Xbox controller
// The message should be formatted as follows:
// <left joystick x-axis> <left joystick y-axis> <A button> <left trigger> <right trigger>
// the values should be separated by spaces
// the values should be floats except for the A button, which should be an integer
// the A button should be 1 if pressed and 0 if not pressed
// the joystick axes should be between -1 (x-left, y-down) and 1 (x-right, y-up)
// the triggers should be between 0 and 1
XboxInput parseInput(char *msg)
{
    // Variables to hold the parsed values
    float ljx, ljy, lt, rt;
    int a;

    // The delimiter for the input message
    const char delim[2] = " ";
    char *token;

    // Parse each value from the message
    ljx = String(strtok(msg, delim)).toFloat();
    ljy = String(strtok(NULL, delim)).toFloat();
    a = String(strtok(NULL, delim)).toInt();
    lt = String(strtok(NULL, delim)).toFloat();
    rt = String(strtok(NULL, delim)).toFloat();

    // Create a new XboxInput struct and return it
    XboxInput xi = {ljx, ljy, a, lt, rt};
    return xi;
}

// Function to scale a float value to a PWM value (0-255)
int scaleToPWM(float val)
{
    return (int)(255 * val);
}

// Function to scale a float value to a tone frequency
int scaleToTone(float val)
{
    return (int)(20000 * val);
}

// Function to control a motor with a given speed
void driveMotor(double speed, int pwm, int in1, int in2)
{
    if (speed > 0)
    {                                        // Forward direction
        analogWrite(pwm, scaleToPWM(speed)); // Set the motor speed
        digitalWrite(in1, HIGH);             // Set the motor direction
        digitalWrite(in2, LOW);
    }
    else
    {                                         // Backward direction
        analogWrite(pwm, -scaleToPWM(speed)); // Set the motor speed
        digitalWrite(in1, LOW);               // Set the motor direction
        digitalWrite(in2, HIGH);
    }
}

// Function to compute the absolute value of an integer
int abs(int x)
{
    if (x >= 0)
        return x;
    else
        return -x;
}

// Function to drive the motors based on the Xbox controller input
void drive(XboxInput input)
{
    // Debounce the A button for mode switching
    if (input.a && (millis() - last_switched > 350))
    {
        last_switched = millis();
        mode = !mode;                          // Switch the mode
        Log.info("Switched to mode %d", mode); // Log the mode switch
    }

    // Drive the motors based on the current mode
    switch (mode)
    {
    case 0:                                      // Mode 0: Independent control of left and right motors
        driveMotor(input.lt, PWMA, IN1A, IN2A);  // Drive the left motor
        analogWrite(LED1, scaleToPWM(input.lt)); // Set the LED brightness to reflect the left motor speed
        driveMotor(input.rt, PWMB, IN1B, IN2B);  // Drive the right motor
        tone(BUZZER, scaleToTone(input.rt));     // Set the buzzer frequency to reflect the right motor speed
        break;

    case 1: // Mode 1: Differential control of left and right motors for steering

        // Choose the higher trigger value for the main motor speed
        if (input.rt > input.lt)
        {
            // Reduce the intensity for the secondary motor based on the joystick input
            float reduced_intensity = input.rt * (1 - abs(input.ljx));
            if (input.ljx >= 0)
            {
                driveMotor(input.rt, PWMA, IN1A, IN2A);
                driveMotor(reduced_intensity, PWMB, IN1B, IN2B);
                analogWrite(LED1, scaleToPWM(input.rt));
                tone(BUZZER, scaleToTone(reduced_intensity));
            }
            else
            {
                driveMotor(reduced_intensity, PWMA, IN1A, IN2A);
                driveMotor(input.rt, PWMB, IN1B, IN2B);
                analogWrite(LED1, scaleToPWM(reduced_intensity));
                tone(BUZZER, scaleToTone(input.rt));
            }
        }
        else
        {
            // Reduce the intensity for the secondary motor based on the joystick input
            float reduced_intensity = input.lt * (1 - abs(input.ljx));
            if (input.ljx >= 0)
            {
                driveMotor(-input.lt, PWMA, IN1A, IN2A);
                driveMotor(-reduced_intensity, PWMB, IN1B, IN2B);
                analogWrite(LED1, scaleToPWM(input.lt));
                tone(BUZZER, scaleToTone(reduced_intensity));
            }
            else
            {
                driveMotor(-reduced_intensity, PWMA, IN1A, IN2A);
                driveMotor(-input.lt, PWMB, IN1B, IN2B);
                analogWrite(LED1, scaleToPWM(reduced_intensity));
                tone(BUZZER, scaleToTone(input.lt));
            }
        }
        break;
    }
    return;
}

// Set up the system
void setup()
{
    // Set the pin modes for the motor control pins
    pinMode(PWMA, OUTPUT);
    pinMode(PWMB, OUTPUT);
    pinMode(IN1A, OUTPUT);
    pinMode(IN2A, OUTPUT);
    pinMode(IN1B, OUTPUT);
    pinMode(IN2B, OUTPUT);

    // Set the pin modes for the debugging LED and buzzer
    pinMode(BUZZER, OUTPUT);
    pinMode(LED1, OUTPUT);

    // Start a UDP server to listen for incoming packets
    udp.begin(LISTENING_PORT);
    Log.info("Local IP: %s", WiFi.localIP().toString().c_str());         // Log the local IP (Serial)
    Particle.publish("Local IP: %s", WiFi.localIP().toString().c_str()); // Publish the local IP (Cloud)
}

// Main program loop
void loop()
{
    // Check for incoming UDP packets
    int packetSize = udp.parsePacket();
    if (packetSize > 0)
    {
        // Get the IP address of the sender
        IPAddress senderIP = udp.remoteIP();

        // If no connection has been established yet, look for an INIT packet to establish a connection
        if (allowedIP == IPAddress(0, 0, 0, 0))
        {
            char message[packetSize + 1];
            udp.read(message, packetSize);
            message[packetSize] = '\0';
            if (!strcmp(message, "INIT")) // If the message is INIT
            {
                allowedIP = senderIP; // Set the allowed IP to the sender's IP
                int senderPort = udp.remotePort();
                udp.beginPacket(senderIP, senderPort);
                udp.write("ACK"); // Acknowledge the connection
                udp.endPacket();
                Log.info("Established connection with %s", senderIP.toString().c_str()); // Log the new connection
            }
        }
        // If a connection has already been established
        else if (senderIP == allowedIP)
        {
            // Read the message from the packet
            char message[packetSize + 1];
            udp.read(message, packetSize);
            message[packetSize] = '\0';

            // Parse the message into Xbox controller input
            XboxInput input = parseInput(message);

            // Drive the motors based on the parsed input
            drive(input);
        }
    }
}

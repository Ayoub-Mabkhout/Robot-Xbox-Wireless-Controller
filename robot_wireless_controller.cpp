#include <string.h>

#define PWMA D1                             // PWM pin to control the speed of the left motor
#define PWMB D2                             // PWM pin to control the speed of the right motor

#define IN1A D3                             // Pins to control the mode and direction of left motor
#define IN2A D4
#define IN1B D5                             // Pins to control the mode and direction of right motor
#define IN2B D5

#define LED1 D0                             // LED 1 for debugging purposes () => turning left
#define BUZZER A4                           // BUZZER for debugging purposes () => turning right

#define LISTENING_PORT 8888


IPAddress allowedIP(0,0,0,0);
UDP udp;
SerialLogHandler logHandler(57600);
int mode = 0;
long last_switched = millis();

typedef struct{
    float ljx;
    float ljy;
    int a;
    float lt;
    float rt;
} XboxInput;

XboxInput parseInput(char* msg){
    float ljx,ljy,lt,rt;
    int a;
    const char delim[2] = " ";
    char* token;
    
    ljx = String(strtok(msg,delim)).toFloat();
    ljy = String(strtok(NULL,delim)).toFloat();
    a = String(strtok(NULL,delim)).toInt();
    lt = String(strtok(NULL,delim)).toFloat();
    rt = String(strtok(NULL,delim)).toFloat();
    
    XboxInput xi = {ljx,ljy,a,lt,rt};
    return xi;
}

int scaleToPWM(float val){
    return (int)(255*val);
}

int scaleToTone(float val){
    return (int)(20000*val);
}

void driveMotor (double speed, int pwm, int in1, int in2) {    // Drive a certain motor with a certain speed
    if (speed > 0){                                         // Either Forward
        analogWrite(pwm,scaleToPWM(speed));                 // Writing Speed
        digitalWrite(in1,HIGH);                             // IN1 HIGH and IN2 LOW for forward following H-Bridge specification
        digitalWrite(in2,LOW);
    } else {                                                // Or Backward
        analogWrite(pwm,-scaleToPWM(speed));                // Writing Speed
        digitalWrite(in1,LOW);                              // IN1 LOW and IN2 HIGH for backward following H-Bridge specification
        digitalWrite(in2,HIGH);
    }
}

int abs(int x){
    if(x >= 0)
        return x;
    else
        return -x;
}

void drive(XboxInput input) {
    
    if(input.a && (millis()-last_switched > 350)){           // debouncing controller button
        last_switched = millis();
        mode = !mode;
        Log.info("Switched to mode %d",mode);
    }
    
    switch(mode){
        case 0:
            driveMotor(input.lt,PWMA,IN1A,IN2A);            // driving left motor
            analogWrite(LED1,scaleToPWM(input.lt));         // LED1 intensity corresponds to left motor intensity
            driveMotor(input.rt,PWMB,IN1B,IN2B);            // driving right motor
            tone(BUZZER,scaleToTone(input.rt));             // BUZZER frequency corresponds to right motor intensity
            break;
        
        case 1:

            if(input.rt > input.lt){
                float reduced_intensity = input.rt*(1-abs(input.ljx));
                if(input.ljx >= 0){
                    driveMotor(input.rt,PWMA,IN1A,IN2A);
                    driveMotor(reduced_intensity,PWMB,IN1B,IN2B);
                    analogWrite(LED1,scaleToPWM(input.rt));
                    tone(BUZZER,scaleToTone(reduced_intensity));
                } else{
                    driveMotor(reduced_intensity,PWMA,IN1A,IN2A);
                    driveMotor(input.rt,PWMB,IN1B,IN2B);
                    analogWrite(LED1,scaleToPWM(reduced_intensity));
                    tone(BUZZER,scaleToTone(input.rt));
                }
            }
            else{
            float reduced_intensity = input.lt*(1-abs(input.ljx));
                if(input.ljx >= 0){
                    driveMotor(-input.lt,PWMA,IN1A,IN2A);
                    driveMotor(-reduced_intensity,PWMB,IN1B,IN2B);
                    analogWrite(LED1,scaleToPWM(input.lt));
                    tone(BUZZER,scaleToTone(reduced_intensity));
                } else{
                    driveMotor(-reduced_intensity,PWMA,IN1A,IN2A);
                    driveMotor(-input.lt,PWMB,IN1B,IN2B);
                    analogWrite(LED1,scaleToPWM(reduced_intensity));
                    tone(BUZZER,scaleToTone(input.lt));
                }
            }
            break;
    }
    return;
}

void setup() {
    // Set up the output pins
    pinMode(PWMA,OUTPUT);
    pinMode(PWMB,OUTPUT);
    pinMode(IN1A,OUTPUT);
    pinMode(IN2A,OUTPUT);
    pinMode(IN1B,OUTPUT);
    pinMode(IN2B,OUTPUT);

    // LED and Buzzer for additional debugging
    pinMode(BUZZER,OUTPUT);
    pinMode(LED1,OUTPUT);

    
    // Set up a UDP socket to listen for incoming packets
    udp.begin(LISTENING_PORT);
    Log.info("Local IP: %s", WiFi.localIP().toString().c_str());
    Particle.publish("Local IP: %s", WiFi.localIP().toString().c_str());
}

void loop() {
    // Check for an incoming packet
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        // Get the IP address of the sender
        IPAddress senderIP = udp.remoteIP();
        // First establish connection with the sender. A Connection is initiated by a request  
        if(allowedIP == IPAddress(0,0,0,0)){

            char message[packetSize+1];
            udp.read(message, packetSize);
            message[packetSize] = '\0';
            if(!strcmp(message,"INIT")){
                allowedIP = senderIP;
                int senderPort = udp.remotePort();
                udp.beginPacket(senderIP,senderPort);
                udp.write("ACK");
                udp.endPacket();
                Log.info("Established connection with %s", senderIP.toString().c_str());
            }

        }
        
        // Check if the sender's IP address matches the one we want to filter
        else if (senderIP == allowedIP) {
            // Read the message from the packet
            char message[packetSize+1];
            udp.read(message, packetSize);
            message[packetSize] = '\0';

            XboxInput input = parseInput(message);
            // Log.info("%f-%f-%d-%f-%f",input.ljx,input.ljy,input.a,input.lt,input.rt);
            drive(input);
        }
    }
}
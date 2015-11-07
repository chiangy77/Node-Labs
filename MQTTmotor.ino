/*
 Basic MQTT example 
 
  - connects to an MQTT server
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic"
*/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <AFMotor.h>
#include <stdio.h>

#define REF_PIN     2

char temp_string[10];
char outTopic[20];

int pwmPin = 3;

// Update these with values suitable for your network.
byte mac[]    = {  0x90, 0xA2, 0xDA, 0x00, 0x00, 0x16 };
byte server[] = { 130, 102, 129, 175 };
byte ip[]     = { 130, 102, 86, 199 };
//byte ip[]     = { 192, 168, 0, 20 };

char message_buff[100];

const char* id;
int percentage = 0;
const char* source;

void callback(char* topic, byte* payload, unsigned int length) {
  
  int i = 0;
  StaticJsonBuffer<100> jsonBuffer;
  
  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';

  
  String msgString = String(message_buff);
  Serial.println("Payload: " + msgString);
    
  JsonObject& root = jsonBuffer.parseObject(message_buff);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  
  //Assign values to variables
  id = root["id"];
  percentage = root["percentage"];
  source = root["source"];
  sprintf(outTopic, "ilab/%s", source);
  
  Serial.println(id);
  Serial.println(percentage);
  Serial.println(source);  
}

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);
AF_DCMotor motor(2, MOTOR12_64KHZ); // create motor #2, 64KHz pwm

void setup()
{ 
  //Connect to MQTT server and subscribe
  Serial.begin(9600);
  Serial.println("starting");
  Ethernet.begin(mac, ip);
  if (client.connect("noderedInterface")) {
    client.publish("ilab/log","Ardunio is alive!");
    client.subscribe("ilab/motor");
    Serial.println("Opened connection");
  }
  else {
    Serial.println("Did not open connection");
  }
}

void loop()
{  
  //If internet is disconnected, attempt to reconnect
  if (!client.connected()) {
    if (client.connect("noderedInterface")) {
      client.publish("ilab/log","Ardunio is alive!");
      client.subscribe("ilab/motor");
      Serial.println("Opened connection");
    }
    else {
      Serial.println("Did not open connection");
      return;
    }
  }
  int power;
  
  //Stops the motor
  if (percentage == 0){
    motor.setSpeed(0);
  } 
  else if (strcmp(id, "1") == 0) {
    
    //Set the current to the motor
    power = 255*percentage/100;
    motor.setSpeed(power);   
                             
    motor.run(FORWARD);      
    
    //Get temperature and publish to topic
    getCurrentTemp(temp_string);
    Serial.println(temp_string);
    client.publish(outTopic, temp_string);
    delay(1000);
  }
    
  client.loop();
}

//Code for sensor
void OneWireReset (int Pin) // reset.  Should improve to act as a presence pulse
{
  digitalWrite (Pin, LOW);
  pinMode (Pin, OUTPUT);        // bring low for 500 us
  delayMicroseconds (500);
  pinMode (Pin, INPUT);
  delayMicroseconds (500);
}

void OneWireOutByte (int Pin, byte d) // output byte d (least sig bit first).
{
  byte n;

  for (n=8; n!=0; n--)
  {
    if ((d & 0x01) == 1)  // test least sig bit
    {
      digitalWrite (Pin, LOW);
      pinMode (Pin, OUTPUT);
      delayMicroseconds (5);
      pinMode (Pin, INPUT);
      delayMicroseconds (60);
    }
    else
    {
      digitalWrite (Pin, LOW);
      pinMode (Pin, OUTPUT);
      delayMicroseconds (60);
      pinMode (Pin, INPUT);
    }

    d = d>>1; // now the next bit is in the least sig bit position.
  }
}


byte OneWireInByte (int Pin) // read byte, least sig byte first
{
  byte d, n, b;

  for (n=0; n<8; n++)
  {
    digitalWrite (Pin, LOW);
    pinMode (Pin, OUTPUT);
    delayMicroseconds (5);
    pinMode (Pin, INPUT);
    delayMicroseconds (5);
    b = digitalRead (Pin);
    delayMicroseconds (50);
    d = (d >> 1) | (b<<7); // shift d to right and insert b in most sig bit position
  }
  return (d);
}


void getCurrentTemp (char *temp)
{
  int HighByte, LowByte, TReading, Tc_100, sign, whole, fract;

  OneWireReset (REF_PIN);
  OneWireOutByte (REF_PIN, 0xcc);
  OneWireOutByte (REF_PIN, 0x44); // perform temperature conversion, strong pullup for one sec

  OneWireReset (REF_PIN);
  OneWireOutByte (REF_PIN, 0xcc);
  OneWireOutByte (REF_PIN, 0xbe);

  LowByte = OneWireInByte (REF_PIN);
  HighByte = OneWireInByte (REF_PIN);
  TReading = (HighByte << 8) + LowByte;
  sign = TReading & 0x8000;  // test most sig bit
  if (sign) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

  whole = Tc_100 / 100;  // separate off the whole and fractional portions
  fract = Tc_100 % 100;

  if (sign) {
    temp[0] = '-';
  } else {
    temp[0] = '+';
  }

  if (whole/100 == 0) {
    temp[1] = ' ';
  } else {
    temp[1] = whole/100+'0';
  }

  temp[2] = (whole-(whole/100)*100)/10 +'0' ;
  temp[3] = whole-(whole/10)*10 +'0';
  temp[4] = '.';
  temp[5] = fract/10 +'0';
  temp[6] = fract-(fract/10)*10 +'0';
  temp[7] = '\0';
}

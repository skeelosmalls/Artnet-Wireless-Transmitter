
/*
Chaned some things for Arduino 1.0

ARTNET SENDER

This SCRIPT allows you to use arduino with ethernet shield or wifi shield and send dmx artnet data. 
Up to you to use logics for channels as you want.

It works with Arduino 023 software

If you have implemented ameliorations to this sketch, please, contribute by sending back modifications, ameliorations, derivative sketch. 
It will be a pleasure to let them accessible to community

This sketch is part of white cat lighting board suite: /http://www.le-chat-noir-numerique.fr  
wich is sending data to many different types of devices, and includes a direct communication in serial also with arduino as devices
You may find whitecat interresting because its theatre based logic ( cuelist and automations) AND live oriented ( masters, midi, etc)

(c)Christoph Guillermet
http://www.le-chat-noir-numerique.fr
karistouf@yahoo.fr
*/
#define VERBOSE
#include <SPI.h>         // needed for Arduino versions later than 0018
#include <ESP8266WiFi.h>  // uses wemos D1 R2
#include <WiFiUdp.h>
#include <WiFiClient.h>
//#include <Ethernet.h>
//#include <EthernetUdp.h>       // UDP library from: bjoern@cs.stanford.edu 12/30/2008

//MAC and IP of the ethernet shield
//MAC adress of the ethershield is stamped down the shield
//to translate it from hexa decimal to decimal, use: http://www.frankdevelopper.com/outils/convertisseur.php
uint8_t MAC_array[6];
String MAC_address;

//TO EDIT:

char wifiSSID[30] = "SSID";
char wifiPass[30] = "PASSWORD";

char nodeName[30] = "artNetNode1";

// the next two variables are set when a packet is received
byte destination_Ip[]= {   255,255,255,255 };        // the ip to send data, 255,255,255,255 is broadcast sending
// art net parameters
unsigned int localPort = 6454;      // artnet UDP port is by default 6454
const int DMX_Universe=0;//universe is from 0 to 15, subnet is not used
const int number_of_channels=512; //512 for 512 channels, MAX=512

//HARDWARE
byte mac[] = {  144, 162, 218, 00, 16, 96  };//the mac adress of ethernet shield or uno shield board
//byte ip[] = {   192,169,0,95 };// the IP adress of your device, that should be in same universe of the network you are using, here: 192.168.1.x

IPAddress ap_ip(2, 0, 0, 10);
IPAddress ip(192,169,0,95);
IPAddress subnet(255, 0, 0, 0);
IPAddress broadcast_ip(ip[0], 255, 255, 255);

//ART-NET variables
char ArtNetHead[8]="Art-Net";
const int art_net_header_size=17;

short OpOutput= 0x5000 ;//output

byte buffer_dmx[number_of_channels]; //buffer used for DMX data


#define ARTNET_PORT 0x1936
#define ARTNET_BUFFER_MAX 600
#define ARTNET_REPLY_SIZE 239
#define ARTNET_ADDRESS_OFFSET 18
#define ARTNET_ARTDMX 0x5000
#define ARTNET_ARTPOLL 0x2000


WiFiUDP eUDP;

//Artnet PACKET
byte  ArtDmxBuffer[(art_net_header_size+number_of_channels)+8+1];


void startWifi() {
  #ifdef VERBOSE
    Serial.print("SSID: ");
    Serial.println(wifiSSID);
    Serial.print("Password: ");
    Serial.println(wifiPass);
    Serial.print("Wifi Connecting");
  #endif

  // Connect wifi
  WiFi.begin(wifiSSID, wifiPass);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(nodeName);

  uint16_t x = 0;
  // Wait for WiFi to connect
  while (WiFi.status() != WL_CONNECTED) {
    // delay to yield to esp core functions
    delay(500);
    #ifdef VERBOSE
      Serial.print(".");
    #endif
  }
  // Get MAC Address
  getMac();

  
    ap_ip = WiFi.localIP();
    ip = WiFi.localIP();
    subnet = WiFi.subnetMask();
  
  
  // Set our broadcast address
  setBroadcastAddr();
  
  #ifdef VERBOSE
    Serial.println(" connected");
    Serial.print("IP: ");
    Serial.println(ip);
    Serial.print("Broadcast Address: ");
    Serial.println(broadcast_ip);
    Serial.print("Subnet: ");
    Serial.println(subnet);
    Serial.print("MAC Address: ");
    Serial.println(MAC_address);
  #endif
}

/* getMac()
 *  This gets the MAC address and formats it for later.
 */
void getMac() {
  char MAC_char[30] = "";
  
  WiFi.macAddress(MAC_array);
  
  // Format the MAC address into string
  sprintf(MAC_char, "%02X", MAC_array[0]);
  for (int i = 1; i < 6; ++i)
    sprintf(MAC_char, "%s : %02X", MAC_char, MAC_array[i]);
  MAC_address = String(MAC_char);
}


/* setBroadcastAddr()
 *  Calculates and sets the broadcast address using the IP and subnet
 */
void setBroadcastAddr() {
  broadcast_ip = {~subnet[0] | (ip[0] & subnet[0]), ~subnet[1] | (ip[1] & subnet[1]), ~subnet[2] | (ip[2] & subnet[2]), ~subnet[3] | (ip[3] & subnet[3])};
}

void setup() {
   #ifdef VERBOSE
    Serial.begin(115200);
    Serial.println();
    Serial.println("ArtnetDMX");
 
  #endif
  
  //initialise artnet header
  construct_arnet_packet();
  // démarrage ethernet et serveur UDP
  startWifi();
  
  // Start listening for UDP packets
  eUDP.begin(ARTNET_PORT);
}

void loop() {
  
   check_arduino_inputs();
   construct_arnet_packet();
   
   eUDP.beginPacket(destination_Ip, localPort);
   eUDP.write(ArtDmxBuffer,(art_net_header_size+number_of_channels+1)); // was Udp.sendPacket
   
   eUDP.endPacket();
   #ifdef VERBOSE
    Serial.print("ArtDmxBuffer sent ");
    #endif
    
   delay(40);
}

void check_arduino_inputs()
{
 //data from arduino aquisition

  int temp_val=0;
  for(int i=0;i<6;i++)//reads the 6 analogic inputs and set the data from 1023 steps to 255 steps (dmx)
  {
    temp_val=analogRead(i); 
    buffer_dmx[i]=byte(temp_val/4.01);
    #ifdef VERBOSE
    Serial.print("temp_val: ");
    Serial.println(temp_val);
    #endif
  }
}


void construct_arnet_packet()
{
     //preparation pour tests
    for (int i=0;i<7;i++)
    {
    ArtDmxBuffer[i]=ArtNetHead[i];
    }   

    //Operator code low byte first  
     ArtDmxBuffer[8]=OpOutput;
     ArtDmxBuffer[9]= OpOutput >> 8;
     //protocole
     ArtDmxBuffer[10]=0;
     ArtDmxBuffer[11]=14;
     //sequence
     ArtDmxBuffer[12]=0;
     //physical 
     ArtDmxBuffer[13] = 0;
     // universe 
     ArtDmxBuffer[14]= DMX_Universe;//or 0
     ArtDmxBuffer[15]= DMX_Universe>> 8;
     //data length
     ArtDmxBuffer[16] = number_of_channels>> 8;
     ArtDmxBuffer[17] = number_of_channels;
   
     for (int t= 0;t<number_of_channels;t++)
     {
       ArtDmxBuffer[t+art_net_header_size+1]=buffer_dmx[t];    
     }
     
}


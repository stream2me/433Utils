/*
    RF 433 messages to MQTT bridge.

    Copyright (c) 2020 Wojciech Sawasciuk <voyo@conserit.pl>
    Distributed under the MIT license.
    
*/

#include "../rc-switch/RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string> 
#include <streambuf>
#include <sstream>
#include <iostream>

#include <mosquittopp.h>
#include <pugixml.hpp>
     
     
RCSwitch mySwitch;

using namespace std;

char DEBUG = 1;
char mqtt_host[200]  = "localhost";
int  mqtt_port = 1883;
char topic[200] = "domoticz/in";

// Use argv[0] to generate an absolute (if necessary) path to a file in the directory of this exe
string generatePath(const string& s, const string& file) {
    if (s.find("/") != string::npos) {
        return s.substr(0, s.rfind( "/" )) + "/" + file;
    } else if (s.find("\\") != string::npos) {
        return s.substr(0, s.rfind("\\")) + "\\" + file;
    } else {
        return file;
    }
}


class RF2mqttWrapper : public mosqpp::mosquittopp
{
public:
	RF2mqttWrapper(const char *id, const char *host, int port);
	~RF2mqttWrapper();

	void on_connect(int rc);
	void on_message(const struct mosquitto_message *message);
	void on_subcribe(int mid, int qos_count, const int *granted_qos);
};
 

RF2mqttWrapper::RF2mqttWrapper(const char *id, const char *host, int port) : mosquittopp(id)
{
	mosqpp::lib_init();			// Initialize libmosquitto

	int keepalive = 120; // seconds
	connect(host, port, keepalive);		// Connect to MQTT Broker
}

RF2mqttWrapper::~RF2mqttWrapper()
{
}


void RF2mqttWrapper::on_message(mosquitto_message const*)
{
}

void RF2mqttWrapper::on_connect(int rc)
{
	printf("Connected with code %d. \n", rc);

	if (rc == 0)
	{
		subscribe(NULL, "command/IoT");
	}
}


void RF2mqttWrapper::on_subcribe(int mid, int qos_count, const int *granted_qos)
{
	printf("Subscription succeeded. \n");
}


int main(int argc, char *argv[]) {
    pugi::xml_document doc;
  
 int res =0;

/*MQTT related configuration from XML config */
    const string config_file = generatePath(argv[0], "config.xml");
    if (!doc.load_file(config_file.c_str() )) return -1;
    pugi::xml_node mqtt_server = doc.child("configuration").child("mqtt_server");
    strcpy(mqtt_host,mqtt_server.attribute("host").value());
    mqtt_port = mqtt_server.attribute("port").as_int();
    strcpy(topic,mqtt_server.attribute("topic").value());

/* XML to 433 values map - "entries" */
    pugi::xml_node entries = doc.child("configuration").child("entries");
    
    std::cout << "XML config loaded correctly.\n";

    // This pin is not the first pin on the RPi GPIO header!
    // Consult https://projects.drogon.net/raspberry-pi/wiringpi/pins/
    // for more information.
    int PIN = 2;
    char tstr[500];
     
    stringstream ss;
    std::string s433;
    std:string msg;

     
    if(wiringPiSetup() == -1) {
      printf("wiringPiSetup failed, exiting...");
      return 0;
    }

    int pulseLength = 0;
    if (argv[1] != NULL) pulseLength = atoi(argv[1]);

     RF2mqttWrapper * mqttHdl;
     mqttHdl = new RF2mqttWrapper("RF2mqtt", mqtt_host,mqtt_port);


     mySwitch = RCSwitch();
     if (pulseLength != 0) mySwitch.setPulseLength(pulseLength);
     mySwitch.enableReceive(PIN);  // Receiver on interrupt 0 => that is pin #2
     
    
     while(1) {
  
      if (mySwitch.available()) {
    
        int value = mySwitch.getReceivedValue();
    
        if (value == 0) {
          printf("Unknown encoding\n");
        } else {    
   
          ss << mySwitch.getReceivedValue();
          string str = ss.str();
          
          s433 = std::to_string( mySwitch.getReceivedValue() );
          msg = entries.find_child_by_attribute("entry", "value", s433.c_str() ).child_value("mqtt_msg");

          if (DEBUG)
             cout << "Received " << mySwitch.getReceivedValue() << ", publishing to mqtt: " << entries.find_child_by_attribute("entry", "value", s433.c_str() ).child_value("mqtt_msg") << "\n"; 

          
  	  mqttHdl->publish(NULL, topic, msg.length(), msg.c_str());	// Publish the data to MQTT topic
        }
    
        fflush(stdout);
        mySwitch.resetAvailable();
      }
      
      res = mqttHdl->loop();						// Keep MQTT connection		
	if (res)
		mqttHdl->reconnect();
      
      
      usleep(10); 
  
  } //while 

  exit(0);


}


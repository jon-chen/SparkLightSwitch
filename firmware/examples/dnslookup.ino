

#include "dnsclient.h"  //include the Library


IPAddress dnsServerIP(8,8,8,8);  			//Set the DNS server to use for the lookup
char serverName[] = "api.pushingbox.com";  	//This is the server we are looking for the IP of
IPAddress remote_addr;   					//This is where we save the IP Address
DNSClient dns;  							//This sets up an instance of the DNSClient



void setup() {
    Serial.begin(9600);						//Turn on the Serial port so we can see the example
    while (!Serial.available()) SPARK_WLAN_Loop();  //Wait for user to press key (for windows users)
    while (Serial.available()) Serial.read(); //throw it away
    
    Serial.print("Host to lookup: ");
    Serial.println(serverName);
    
    int ret = 0;
    dns.begin(dnsServerIP);					//this send the DNS server to the library
    ret = dns.getHostByName(serverName, remote_addr);	//ret is the error code, getHostByName needs 2 args, the server and somewhere to save the Address
    if (ret == 1) {
        Serial.print("Host to lookup: ");
        Serial.println(serverName);
        Serial.print("IP Address is: ");
        Serial.println(remote_addr);
    } else {
        Serial.println("Error code: ");
        Serial.println(ret);
    }
	
//    Error Codes
//SUCCESS          1
//TIMED_OUT        -1
//INVALID_SERVER   -2
//TRUNCATED        -3
//INVALID_RESPONSE -4

}

void loop() {

}
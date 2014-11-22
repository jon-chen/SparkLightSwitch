DNSClient
=========

DNS Client for Spark. 


Original code from Arduino Ethernet library, ported for use on Spark Core

the myUDP.h was written by a unknown person on the Spark Forums and overcomes an issue 
with sending a UDP packet each time the write is called, now it is only sent 
when endPacket is called

Example inluded, change the dnsServerIP to whatever DNS server you want to use
and change serverName to the place you want to look up.

Thanks to everyone who helped make this work


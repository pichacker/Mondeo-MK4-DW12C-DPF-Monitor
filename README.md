# Mondeo-MK4-DW12C-DPF-Monitor
An LED Monitor For the DW12C Mondeo DPF / Vapouriser

I was approached by a colleage asking how often my DFP regenerated on my 2.2L Diesel MK4 (MCA) Ford Mondeo. True answer was that I hadn't thought about it and wasn't too concerned provided the car carried on running and it didn't clog up.

My colleague had wired an LED to the Vapouriser Glow Plug Relay which he brought back into the cabin. I didn't like this approach and thought that I could do better by monitoring the CAN messages at the diagnostic connector inside the car. 

This project was born....It uses easily obtainable, cheap, components from the web to create a device that reads parameters from the engine ECU and lights LEDs appropriately. 

Libraries used are the MCP2515 Driver and SPI. I believe that I used the 107-Arduino-MCP2525 by Alexander Entinger https://github.com/107-systems/107-Arduino-MCP2515[Project.zip](https://github.com/pichacker/Mondeo-MK4-DW12C-DPF-Monitor/files/9887955/Project.zip)


Components required are LEDs, 330R series resistors, small voltage regulator, MCP2515 CAN board (8Mhz), Arduino Pro Micro (5v). And of course a suitable box to stuff it all in. You can decide how to connect to the cars CAN network.

The voltage regulator used was one of the adjustable ones and I removed the variable control and found that a 1k5 resistor gave me about 6.5V which was adequate to feed into the RAW pin of the pro-micro.

Connect as below.
12V to regulator.
Ground connections from regulator to CAN board, Pro Micro, and LEDs.
Output from regulator to RAW input of Pro Micro

VCC of Pro Micro to VCC of CAN board
Pro Micro 10 to CS of CAN board
Pro Micro 16 to SO of CAN board
Pro Micro 14 to SI of CAN board
Pro Micro 15 to SCK of CAN board

LEDs with 330R series resistor.
Pro Micro 2 to Glow Plug
Pro Micro 3 to Metering Pump
Pro Micro 4 to 0% Bargraph
Pro Micro 5 to 20% Bargraph
Pro Micro 6 to 40% Bargraph
Pro Micro 7 to 60% Bargraph
Pro Micro 8 to 80% Bargraph
Pro Micro 9 to 100% Bargraph

No need for terminating link on CAN board as vehicle network already terminated.
CAN Board H to Diagnostic connector pin 6
CAN Board L to Diagnostic connector pin 14
Gound lead to pins 4 & 5 of Diagnostic connector
+12V supply from pin 16 of Diagnostic connector

The display will show a calcualted percentage of soot loading from 0 to 100% derived from reading CAN DIDs 057B and 0579
An LED to show when glow plug has been commanded ON by ECU by reading DID 03B8
An LED that will show operation of the Vapouriser pump frequency by reading DID 041E

SC 28/10/2022

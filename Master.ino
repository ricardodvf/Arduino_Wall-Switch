/**
   The nodes can change physical or logical position in the network, and reconnect through different
   routing nodes as required. The master node manages the address assignments for the individual nodes
   in a manner similar to DHCP.
*/

#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <EEPROM.h>
#include <Console.h>
#include <Bridge.h>



/***** Configure the chosen CE,CS pins *****/
RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

uint32_t displayTimer = 0;
char dat[20];
//Data Structs to store information to send and receive.
// M is for Master Information.  Since this is the Master we will use it to send it.
// N is for Nodes Information.  Since this is the Master it will be used to store the data received.
typedef struct {
  int N_Sender;
  int N_Receiver;
  char N_Command;
  int N_InfoNum1;
  int N_InfoNum2;
  int N_InfoNum3;
  char N_Other[5];
} A_1;

typedef struct {
  int M_Sender;
  int M_Receiver;
  char M_Command;
  int M_InfoNum1;
  int M_InfoNum2;
  int M_InfoNum3;
  char M_Other[5];
} A_2;

A_1 Receive;
A_2 Send;

//*********************** VOID SETUP  *************************************************************************
//***********************************************************************************************************
void setup() {
  Bridge.begin();
  Console.begin();
  pinMode(2, INPUT_PULLUP);
  digitalWrite(2, LOW);
  delay(1000);
  Bridge.begin();
  Console.begin();

  // Set the nodeID to 0 for the master node
  mesh.setNodeID(0);

  // Connect to the mesh
  mesh.begin();
  Console.println(mesh.getNodeID());
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS); 


  Send.M_Sender = 0;
}

//*********************** VOID LOOP *********************************************************************************
//******************************************************************************************************************
void loop() {

  // Call mesh.update to keep the network updated
  mesh.update();

  // In addition, keep the 'DHCP service' running on the master node so addresses will
  // be assigned to the sensor nodes
  mesh.DHCP();

  // Check for commands from Serial monitor

  if ( Console.available() ) {
   
    char CMD[1];
    char Trash[1];
    CMD[0] = Console.read();
    do {
      Trash[0] = Console.read();
    } while(Console.available() > 0);

    Send.M_Receiver = 3;
    Send.M_Command = CMD[0];
    Send.M_InfoNum1 = 55;
    Send.M_InfoNum2 = 23;
    Send.M_InfoNum3 = 23;
    Send.M_Other[0] = 'H';
    Send.M_Other[1] = 'o';
    Send.M_Other[2] = 'l';
    Send.M_Other[3] = 'a';
    Send.M_Other[4] = '!';
    



    bool TX = true;
    int r = 0;
    do {
      if (mesh.write(&Send, 'S', sizeof(Send), Send.M_Receiver)) {
        Console.println(F("Sent: "));
        Console.print(F("Sender: ")); Console.println(Send.M_Sender);
        Console.print(F("Receiver: ")); Console.println(Send.M_Receiver);
        Console.print(F("Command: ")); Console.println(Send.M_Command);
        Console.print(F("InfoNum1: ")); Console.println(Send.M_InfoNum1);
        Console.print(F("InfoNum2: ")); Console.println(Send.M_InfoNum2);
        Console.print(F("InfoNum3: ")); Console.println(Send.M_InfoNum3);
        Console.print(F("Other: ")); Console.println(Send.M_Other);
        TX = true;
      }
      else {
        Console.println(F("Tranmission Failed"));
        Console.print(F("Sender: ")); Console.println(Send.M_Sender);
        Console.print(F("Receiver: ")); Console.println(Send.M_Receiver);
        Console.print(F("Command: ")); Console.println(Send.M_Command);
        Console.print(F("InfoNum1: ")); Console.println(Send.M_InfoNum1);
        Console.print(F("InfoNum2: ")); Console.println(Send.M_InfoNum2);
        Console.print(F("InfoNum3: ")); Console.println(Send.M_InfoNum3);
        Console.print(F("Other: ")); Console.println(Send.M_Other);
        TX = false;
      }
      if (TX == true) {
        break;
      }
      r = r + 1;
    } while (r < 2);


  }

  // Check for incoming data from the sensors
  if (network.available()) {
    RF24NetworkHeader header;
    network.peek(header);

    switch (header.type) {
      // Display the incoming values from the sensor nodes
      case 'S': network.read(header, &dat, sizeof(dat)); Console.print(Translate()); Console.print(F(" Motion: ")); Console.println(digitalRead(2)); break;
      default: network.read(header, 0, 0); Console.println(header.type); break;
    }
  }

  if (millis() - displayTimer > 5000) {
    displayTimer = millis();
    Console.println(" ");
    Console.println(F("********Assigned Addresses********"));
    for (int i = 0; i < mesh.addrListTop; i++) {
      Console.print("NodeID: ");
      Console.print(mesh.addrList[i].nodeID);
      Console.print(" RF24Network Address: 0");
      Console.println(mesh.addrList[i].address, OCT);
    }
    Console.println(F("**********************************"));
  }
}

//************************************************* TRANSLATE FUNCTION **************************************************************
//************************************************************************************************************************************

String Translate() {

  String next = "";
  String tr = "";

  for (int i = 0; i < 8; i = i + 1) {

    tr = tr + int(dat[i]);
  }

  return tr;
}

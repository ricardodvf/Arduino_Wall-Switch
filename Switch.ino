#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <EEPROM.h>
#include <avr/interrupt.h>

#define nodeType 1

/**** Configure the nrf24l01+ CE and CS pins ****/
RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);



volatile int RelayPIN = 2;  // Pin used to to control the 110 V relay
int SwitchPIN = 3; // Pin to read the state of the physical light switch
int SwitchState = 0; // Variable to store the state of the physical light swtich
int PrevSwitchState = 0; // Variable to remember the position of the switch
volatile int RelayState = 0;  // Variable to record the state of the relay everytime we change it.
uint32_t displayTimer = 0; //To record last time of some events
int nodeID = 3; // NodeID for NRF24L01
volatile static unsigned long last_interrupt_time = 0; // For debouncing purposes, records last time of a swithc change.
int Errors = 0;
//Data Structs to store information to send and receive.
// M is for Master Information.  Since this is a Node it will be used to store received data.
// N is for Nodes Information.  Since this is a Node we will use it to send it.
typedef struct {
  int M_Sender;
  int M_Receiver;
  char M_Command;
  int M_InfoNum1;
  int M_InfoNum2;
  int M_InfoNum3;
  char M_Other[5];
} A_1;

typedef struct {
  int N_Sender;
  int N_Receiver;
  char N_Command;
  int N_InfoNum1;
  int N_InfoNum2;
  int N_InfoNum3;
  char N_Other[5];
} A_2;

A_1 Receive;
A_2 Send;

//************* VOID SETUP BEINGS  ***************************************************************************
//**********************************************************************************************************

void setup() {
  sei();
  pinMode(RelayPIN, OUTPUT);
  pinMode(SwitchPIN, INPUT_PULLUP);
  digitalWrite(RelayPIN, HIGH);
  attachInterrupt(digitalPinToInterrupt(SwitchPIN), SwitchActivated, CHANGE);
  RelayState = 1;

  Send.N_Sender = nodeID;


  //nodeID = EEPROM.read(5);

  //EEPROM.write(5, 2);


  Serial.begin(115200);
  // EEPROMStatus();  Enable this to read the values of EEPROM memory using the Serial Monitor
  // Set the nodeID
  mesh.setNodeID(nodeID);


  // Connect to the mesh
  Serial.print(F("Node ID = "));
  Serial.println(nodeID);
  Serial.println(F("Connecting to the mesh..."));
  PrevSwitchState = digitalRead(SwitchPIN);

  mesh.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
}
//**********************INTERRUPT SERVICE ROUTINE*****************************************************************
//*****************************************************************************************************************

void SwitchActivated() {
  unsigned long interrupt_time = millis();
  int RelaySt = bitRead(PORTD, RelayPIN);

  if (interrupt_time - last_interrupt_time > 20) {
    if (RelaySt == 0) {
      digitalWrite(RelayPIN, HIGH);
    }
    else {
      digitalWrite(RelayPIN, LOW);
    }
  }
  last_interrupt_time = interrupt_time;
}
//************* VOID LOOP BEGINS ******************************************************************************
//*************************************************************************************************************

void loop() {

  Wireless();
}

//*************************************************************************************************************
//*************  WIRELESS FUNCTION  ***************************************************************************

void Wireless() {

  SwitchState = digitalRead(SwitchPIN);
  RelayState = bitRead(PORTD, RelayPIN);


  mesh.update();
  // Only executes every second ******** Sends status to master node
  if (millis() - displayTimer >= 1000) {

    Serial.print(F("Switch is: "));Serial.print(SwitchState);
    Serial.print(F("  Previous Switch is: "));Serial.print(PrevSwitchState);
    Serial.print(F("  Relay is: "));Serial.print(RelayState);
    Serial.print(F("  Errors in Transmission: "));Serial.println(Errors);

    displayTimer = millis();

    Send.N_Receiver = 0;
    Send.N_Command = 0;

    if ( RelayState == 1) {
      Send.N_InfoNum1 = 0;

    }

    else {
      Send.N_InfoNum1 = 1;

    }

    // Send an 'S' type message
    if (!mesh.write(&Send, 'S', sizeof(Send))) {

      // If a write fails, check connectivity to the mesh network
      if ( ! mesh.checkConnection() ) {
        //refresh the network address
        Serial.println(F("Renewing Address"));
        mesh.renewAddress();
      } else {
        Serial.println(F("Send fail, Test OK"));
        Errors = Errors + 1;
      }
    } else {
      Serial.print(F("Node ID = "));
      Serial.print(nodeID);
      Serial.print(F("  "));
      Serial.print(F("Send OK: "));
      Serial.print(F("  Switch is: "));
      Serial.print(digitalRead(SwitchPIN));
      Serial.print(F("  Relay is: "));
      Serial.println(digitalRead(RelayPIN));
      Serial.println();
    }
  }

  // Checks for new information in the network****** this is done every loop
  while (network.available()) {

    RF24NetworkHeader header;
    network.read(header, &Receive, sizeof(Receive));
    Serial.println(F("Received packet******** "));
    Serial.print(F("Sender: ")); Serial.println(Receive.M_Sender);
    Serial.print(F("Receiver: ")); Serial.println(Receive.M_Receiver);
    Serial.print(F("Command: ")); Serial.println(Receive.M_Command);
    Serial.print(F("InfoNum1: ")); Serial.println(Receive.M_InfoNum1);
    Serial.print(F("InfoNum2: ")); Serial.println(Receive.M_InfoNum2);
    Serial.print(F("InfoNum3: ")); Serial.println(Receive.M_InfoNum2);
    Serial.print(F("Other: ")); Serial.println(Receive.M_Other);
    
    Serial.println(F("***************** "));
    if (millis() - last_interrupt_time > 20) {
      if (Receive.M_Command == 'P') {
        digitalWrite(RelayPIN, LOW);
        last_interrupt_time = millis();
        Serial.println(F("LED Prendido"));
      }
      if (Receive.M_Command == 'A') {
        digitalWrite(RelayPIN, HIGH);
        last_interrupt_time = millis();
        Serial.println(F("LED Apagado"));
      }
    }
  }
}


// ************************************************************************************************************************
// *********************** VOID EEPROMStatus*******************************************************************************
// Prints all EEPROM memory values to the serial monitor

void EEPROMStatus() {

  for (int i = 0; i < 255 ; i = i + 1) {
    Serial.print(F("EEPROM Mem # "));
    Serial.print(i);
    Serial.print(F(" : "));
    Serial.println(EEPROM.read(i));
  }

}
// ************************************************************************************************************************
//************************RESET FUNCTION***********************************************************************************
void(* resetFunc) (void) = 0;


//************************CHANGE NODE ID FUNCTION **************************************************************************
//**************************************************************************************************************************
void ChangeNodeID(int newNode) {
  EEPROM.write(5, newNode);
  nodeID = newNode;
  resetFunc();
}



#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <vector> 

#define SS_PIN 5  // ESP32 pin GPIO5 
#define RST_PIN 21 // ESP32 pin GPIO27 
#define RED_LED 12 
#define GREEN_LED 14
#define ADD_BUTTON 33
#define REMOVE_BUTTON 32
std::vector<String> allowedIDs;

MFRC522DriverPinSimple ss_pin(SS_PIN); // Configurable, see typical pin layout above.

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver.

MFRC522 mfrc522{driver}; // Create MFRC522 instance.

String tagContent = "";

#include "pitches.h"
#define BUZZER_PIN  22 // ESP32 pin GPIO18 connected to piezo buzzer

int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

int melody[] = {
  NOTE_C8, NOTE_A7, NOTE_A7, NOTE_D8, NOTE_C7, 0, NOTE_C8, NOTE_C8
};

void setup() {
  Serial.begin(115200); // Initialize serial communications with the PC for debugging.
  mfrc522.PCD_Init();                                     // Init MFRC522 board.
  pinMode(12, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(ADD_BUTTON, INPUT_PULLUP);
  pinMode(REMOVE_BUTTON, INPUT_PULLUP);
  addCard("73 2E 88 11");
  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
}

//Função que lê o cartão.
void readRfid() {
  String ID = "";
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }

  // Select one of the cards.
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  Serial.println("Imprimindo o ID:");
  Serial.println("--------------------------");
  ID = getID(tagContent);
  permissionStatus(ID);
  Serial.println(ID);
  ID = "";
}

//Isso vai ser importante dps confia
String getID(String tagContent){
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    tagContent.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    tagContent.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  tagContent.remove(tagContent.indexOf(' '), 1); //Remove o primeiro espaço do ID
  tagContent.toUpperCase();
  return tagContent;
}
//Função que libera a tranca
void permissionStatus(String tagContent){
  if(std::find(allowedIDs.begin(), allowedIDs.end(), tagContent) != allowedIDs.end()){
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    Serial.println("true");
    delay(1000);
    digitalWrite(GREEN_LED, LOW);
  }
  else{
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    Serial.println("false");
    bool alarm = true;
    while (alarm){
      for (int thisNote = 0; thisNote < 8; thisNote++) {
      int noteDuration = 1000 / noteDurations[thisNote];
      tone(BUZZER_PIN, melody[thisNote], noteDuration);

      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      noTone(BUZZER_PIN);
      }
      alarm = false;
    }
  }
}

void addCard(String newID) {
  if (std::find(allowedIDs.begin(), allowedIDs.end(), newID) == allowedIDs.end()) {
    allowedIDs.push_back(newID);
    Serial.println("Card added successfully!");
  } else {
    Serial.println("Card is already in the list.");
  }
}

// Função para remover um cartão da lista de IDs permitidos
void removeCard(String removeID) {
  auto it = std::find(allowedIDs.begin(), allowedIDs.end(), removeID);
  if (it != allowedIDs.end()) {
    allowedIDs.erase(it);
    Serial.println("Card removed successfully!");
  } else {
    Serial.println("Card not found in the list.");
  }
}
void loop() {
  /*Serial.println("button 1: ");
  Serial.print(digitalRead(ADD_BUTTON));
  Serial.println("");
  if (digitalRead(ADD_BUTTON) == 0){
    
  }
  Serial.println("button 2: ");
  Serial.print(digitalRead(REMOVE_BUTTON));
  Serial.println("");*/
  readRfid();
}


#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <vector> 
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define BOTtoken "7070421976:AAEdf-M1HGIG6OSbHWFBqKQr0I5U4mJMoFY"
#define CHAT_ID "6240132462"

//Configuração do Wi-Fi
const char* ssid = "sala203";
const char* password = "s@l@203#";


WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

#define SS_PIN 5  // ESP32 pin GPIO5 
#define RST_PIN 21 // ESP32 pin GPIO27 
#define RED_LED 12 
#define GREEN_LED 14
#define ADD_BUTTON 33
#define REMOVE_BUTTON 32
//#define RELAY 26
std::vector<String> allowedIDs;

MFRC522DriverPinSimple ss_pin(SS_PIN); // Configura o driver do sensor RFID

MFRC522DriverSPI driver{ss_pin}; // Cria o driver

MFRC522 mfrc522{driver}; // Cria a instância do sensor.

String tagContent = ""; //String que grava a leitura atual

#include "pitches.h"

#define BUZZER_PIN  22 // Pino do BUzzer

//Função que define o toque do buzzer.
int melody[] = {
  NOTE_C8, NOTE_A7, NOTE_A7, NOTE_D8, NOTE_C7, 0, NOTE_C8, NOTE_C8
};

int melody2[] = {
  NOTE_E7, NOTE_F7, NOTE_E7, NOTE_F7, NOTE_E7, NOTE_F7, NOTE_E7, NOTE_F7
};

//Função que dita a duração das notas.
/*int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};*/
int noteDurations[] = {
  4, 4, 4, 4, 4, 4, 4, 4
};


void setup() {
  Serial.begin(115200); // Comunicação serial para debug.

 WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Conectado ao WiFi");


  mfrc522.PCD_Init(); // Iniciar o sensor RFID
  //Definindo os pinos dos leds como output.
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  //Definindo botões como Input.
  pinMode(ADD_BUTTON, INPUT_PULLUP);
  pinMode(REMOVE_BUTTON, INPUT_PULLUP);
  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
  

  //pinMode(RELAY, OUTPUT);
  //digitalWrite(RELAY, LOW);
}

//Função que lê o cartão.
void readRfid() {
  String ID = "";
  // Reseta o loop se nenhum cartão for apresentado ao sensor.
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }

  // Seleciona o cartão.
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  //Imprime o ID
  Serial.println("Imprimindo o ID:");
  Serial.println("--------------------------");
  ID = getID(tagContent);
  permissionStatus(ID); //Verifica se o ID está cadastrado ou não.
  Serial.println(ID);
  ID = ""; //Limpa a variável ID após a execução.
}

//Função que adquire o ID
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
    bot.sendMessage(CHAT_ID, "O acesso foi liberado, ID:" + tagContent);
    //digitalWrite(RELAY, LOW); //
    delay(1000);
    digitalWrite(GREEN_LED, LOW);
  }
  else{
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    //digitalWrite(RELAY, HIGH); //
    Serial.println("false");
    bool alarm = true;

    bot.sendMessage(CHAT_ID, "Um ID inválido foi apresentado, o alarme foi ativado!");
    while (alarm){
      for (int thisNote = 0; thisNote < 8; thisNote++) {
      int noteDuration = 1000 / noteDurations[thisNote];
      tone(BUZZER_PIN, melody2[thisNote], noteDuration);

      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      noTone(BUZZER_PIN);
      }
      alarm = false;  
    }
    
  }
}

//Função que adiciona novas IDs de Tags ao sistema.
void addCard(String newID) {
  if (std::find(allowedIDs.begin(), allowedIDs.end(), newID) == allowedIDs.end()) {
    allowedIDs.push_back(newID);
    Serial.println("Card added successfully!");
    bot.sendMessage(CHAT_ID, "Um novo ID foi adicionado, ID: " + newID);
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
     bot.sendMessage(CHAT_ID, "Um ID foi removido, ID: " + removeID);
  } else {
    Serial.println("Card not found in the list.");
  }
}

bool registryButtonPressed = false;
bool removeButtonPressed = false;

void loop() {
  //Se o botão de adicionar for pressionado, adiciona ID do próximo cartão que for lido pelo sensor ao sistema.
  if (digitalRead(ADD_BUTTON) == LOW && !registryButtonPressed) {
    registryButtonPressed = true;
    addCard(getID(tagContent));
  } else if(digitalRead(ADD_BUTTON) == HIGH) {
    registryButtonPressed = false;
  }
  //Se o botão de remover for pressionado, remove do sistema o ID do próximo cartão que for lido pelo sensor.
  if (digitalRead(REMOVE_BUTTON) == LOW && !removeButtonPressed){
    removeButtonPressed = true;
    removeCard(getID(tagContent));
  } else if(digitalRead(REMOVE_BUTTON) == HIGH) {
    removeButtonPressed = false;
  }
  //Se nenhum botão for pressionado, operar normalmente.
  if (digitalRead(ADD_BUTTON) == HIGH && digitalRead(REMOVE_BUTTON) == HIGH){
    readRfid();
  }
  
}


#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <vector> 
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "pitches.h"

//Definição do token e ID do Telegram
#define BOTtoken ""
#define CHAT_ID ""

//Configuração do Wi-Fi
const char* ssid = "";
const char* password = "";

//Configurando o Cliente de Wi-Fi e o bot, utilizando o token e ID.
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

//Definindo pinos do Sensor RFID
#define SS_PIN 5  
#define RST_PIN 21

//Definindo pinos dos botões, buzzer e LEDs.
#define RED_LED 12 
#define GREEN_LED 14
#define ADD_BUTTON 33
#define REMOVE_BUTTON 32
#define BUZZER_PIN  22
//#define RELAY 26
std::vector<String> allowedIDs;

// Configurações do driver do sensor RFID
MFRC522DriverPinSimple ss_pin(SS_PIN); 
MFRC522DriverSPI driver{ss_pin}; // Cria o driver
MFRC522 mfrc522{driver}; // Cria a instância do sensor.

String tagContent = ""; //String que grava a leitura atual

//Funções que definem o toque do buzzer.
int melody2[] = {
  NOTE_E7, NOTE_F7, NOTE_E7, NOTE_F7, NOTE_E7, NOTE_F7, NOTE_E7, NOTE_F7
};

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

//Função que lê o cartão.
void readRfid() {
  String ID = "";
  // Reseta o loop se nenhum cartão for apresentado ao sensor.
  if (!mfrc522.PICC_IsNewCardPresent()){
    return;
  }
  // Seleciona o cartão.
  if (!mfrc522.PICC_ReadCardSerial()){
    return;
  }
  Serial.println("Imprimindo o ID:");
  Serial.println("--------------------------");
  ID = getID(tagContent);
  permissionStatus(ID); //Verifica se o ID está cadastrado ou não.
  Serial.println(ID); //Imprime o ID
  ID = ""; //Limpa a variável ID após a execução.
}

//Função que determina se o ID apresentado é válido ou não.
void permissionStatus(String tagContent){
  if(std::find(allowedIDs.begin(), allowedIDs.end(), tagContent) != allowedIDs.end()){
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    Serial.println("true");
    bot.sendMessage(CHAT_ID, "O acesso foi liberado, ID:" + tagContent);
    delay(1000);
    digitalWrite(GREEN_LED, LOW);
  } else {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
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
    Serial.println("Cartão adicionado com sucesso!");
    bot.sendMessage(CHAT_ID, "Um novo ID foi adicionado, ID:" + newID);
  } else {
    Serial.println("Cartão já adicionado!");
  }
}

// Função para remover um cartão da lista de IDs permitidos
void removeCard(String removeID) {
  auto it = std::find(allowedIDs.begin(), allowedIDs.end(), removeID);
  if (it != allowedIDs.end()) {
    allowedIDs.erase(it);
    Serial.println("Cartão removido com sucesso!");
     bot.sendMessage(CHAT_ID, "Um ID foi removido, ID:" + removeID);
  } else {
    Serial.println("O cartão não foi encontrado na lista!");
  }
}

//Variáveis dos botões.
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

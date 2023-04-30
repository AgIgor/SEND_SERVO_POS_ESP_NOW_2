#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define LED_PIN 2
#define SPEAKER 4

#define DEBUG
//#define DEBUG_DATA_SENT
//#define DEBUG_PKG
#define DEBUG_INPUT

#define READ_PIN_A 18
#define PULSE_PIN_A 19

#define READ_PIN_B 22
#define PULSE_PIN_B 23

int freq = 4050, t = 126, dly = 150;

const uint32_t  ANALOG_TIMEOUT  = 500;  // = 20ms 20000

uint8_t broadcastAddress[] = {0x40,0x22,0xD8,0x5F,0xF7,0xFC};// mac metalico
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


// Definir a estrutura para o pacote de dados
typedef struct struct_message {
  int servoA, servoB;
  bool led;
} struct_message;
// Inicializar a estrutura do pacote de dados
struct_message myData;

void bipBuzzer(){
  for(int i=0;i<4;i++){
    tone(SPEAKER, freq, t);
    delay(dly);
    noTone(SPEAKER);
  }
  delay(2000);
}//end bip buzzer

uint8_t analogDigitalRead(const uint8_t Ppin, const uint8_t Rpin) {
  // Envia pulso
  digitalWrite(Ppin, HIGH);
  uint32_t us = micros();

  // Aguarda carga do capacitor
  while ((micros() - us) < ANALOG_TIMEOUT && digitalRead(Rpin) == LOW);

  // Obtem tempo de carga
  us = micros() - us;

  // Coloca pino em nível baixo para descarga do capacitor
  digitalWrite(Ppin, LOW);

  // Aguarda descarga (= Tau x 5)
  delay(ANALOG_TIMEOUT / 1000 * 5); // Tempo em ms

  // Verifica valor lido
  if (us > ANALOG_TIMEOUT) {
    // Timeout
    us = ANALOG_TIMEOUT;
  }
  
  // Calcula leitura (0% a 100%)
  return uint8_t(us * 100 / ANALOG_TIMEOUT);
}//end analogDigitalRead

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) { 
  #ifdef DEBUG_DATA_SENT
    Serial.println(sendStatus == 0 ? "Entregue":"Não entregue");
  #endif
  digitalWrite(LED_PIN, !sendStatus);

  if(sendStatus == 1)bipBuzzer();

}//end OnDataSent

void wifi_init(){
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  #ifdef DEBUG 
    Serial.print("TX WiFi.macAddress: ");
    Serial.print(WiFi.macAddress());
    Serial.print(" Power: ");
    Serial.println(WiFi.getTxPower());
  #endif

  /*
      Available ESP32 RF power parameters:
      WIFI_POWER_19_5dBm    // 19.5dBm (For 19.5dBm of output, highest. Supply current ~150mA)
      WIFI_POWER_19dBm      // 19dBm
      WIFI_POWER_18_5dBm    // 18.5dBm
      WIFI_POWER_17dBm      // 17dBm
      WIFI_POWER_15dBm      // 15dBm
      WIFI_POWER_13dBm      // 13dBm
      WIFI_POWER_11dBm      // 11dBm
      WIFI_POWER_8_5dBm     //  8dBm
      WIFI_POWER_7dBm       //  7dBm
      WIFI_POWER_5dBm       //  5dBm
      WIFI_POWER_2dBm       //  2dBm
      WIFI_POWER_MINUS_1dBm // -1dBm( For -1dBm of output, lowest. Supply current ~120mA)
      Available ESP8266 RF power parameters:
      0    (for lowest RF power output, supply current ~ 70mA
      20.5 (for highest RF power output, supply current ~ 80mA
  */
}//end wifi init

void esp_now_start(){
  if (esp_now_init() != ESP_OK) {
    #ifdef DEBUG 
      Serial.println("Error initializing ESP-NOW");
    #endif
    return;
  }
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  for (int ii = 0; ii < 6; ++ii )
  {
    peerInfo.peer_addr[ii] = (uint8_t) broadcastAddress[ii];
  }
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    #ifdef DEBUG 
      Serial.println("Failed to add peer");
    #endif
    return;
  }  
}//end esp now start

void setup() {
  #ifdef DEBUG 
    Serial.begin(115200);
  #endif

  pinMode(SPEAKER, OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,LOW);

  pinMode(READ_PIN_A,   INPUT);
  pinMode(PULSE_PIN_A,  OUTPUT);

  pinMode(READ_PIN_B,   INPUT);
  pinMode(PULSE_PIN_B,  OUTPUT);

  uint8_t adc = analogDigitalRead(PULSE_PIN_A, READ_PIN_A);
  uint8_t adc2 = analogDigitalRead(PULSE_PIN_B, READ_PIN_B);

  /* while (adc != 1 || adc2 != 1){
    adc = analogDigitalRead(PULSE_PIN_A, READ_PIN_A);
    adc2 = analogDigitalRead(PULSE_PIN_B, READ_PIN_B);
    Serial.print(adc);
    Serial.print(" ");
    Serial.print(adc2);
    Serial.println("");

    delay(500);

    for(int i=0;i<2;i++){
      tone(SPEAKER, freq, t);
      delay(200);
      noTone(SPEAKER);
    }
  } */
  
  wifi_init();
  esp_now_start();
  esp_now_register_send_cb(OnDataSent);

}//end setup

void loop() {

  uint8_t adc = analogDigitalRead(PULSE_PIN_A, READ_PIN_A);
  uint8_t adc2 = analogDigitalRead(PULSE_PIN_B, READ_PIN_B);

  #ifdef DEBUG_INPUT
    Serial.print(adc);
    Serial.print(" ");
    Serial.print(adc2);
    Serial.println("");
  #endif

  delay(1);

  // Construir o pacote de dados
  //strcpy(myData.data, "Hello world");

  myData.servoA = adc;
  myData.servoB = adc2;
  myData.led = !myData.led;

  // Enviar o pacote de dados para o receptor
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  #ifdef DEBUG_PKG
    Serial.println(result == ESP_OK?"PKG_OK":"PKG_ERROR");
  #endif
  //delay(1000);
}//end loop
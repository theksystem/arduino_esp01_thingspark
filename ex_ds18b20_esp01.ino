#include <SoftwareSerial.h>
#include <OneWire.h>

#define DEBUG true

SoftwareSerial esp(2, 3);

int DS18S20_Pin = 4; //DS18S20 Signal pin on digital 4

//Temperature chip i/o
OneWire ds(DS18S20_Pin);  // on digital pin 4

String AP = "WIFI_SSID";
String PASS = "WIFI_PASS";
String HOST = "api.thingspark.co.kr";
String PORT = "8480";
String API = "THINGSPARK_APIKEY";  // 16 자리 api key

void setup(void) {
  Serial.begin(115200);
  esp.begin(115200);

  sendCommand("AT", 5, "OK");
  sendCommand("AT+CWMODE=1", 5, "OK");
  sendCommand("AT+CWJAP=\"" + AP + "\",\"" + PASS + "\"", 20, "OK");
}


float field1_data = 0.0;


unsigned long loopPreviousMillis = 0;
unsigned long loopInterval = 1000;

unsigned long sendPreviousMillis = 0;
const long sendInterval = 30000; // 초(1000) 대기시간

void loop(void) {
  // put your main code here, to run repeatedly:
  if (esp.available()) {
    Serial.write(esp.read());
  }
  if (Serial.available()) {
    esp.write(Serial.read());
  }

  unsigned long currentMillis = millis();
  if (currentMillis  - loopPreviousMillis >= loopInterval) {
    loopPreviousMillis = currentMillis ;

    float temperature = getTemp();
    Serial.println(temperature);
    field1_data = temperature;
  }

  // 데이타 전송 타이머
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - sendPreviousMillis >= sendInterval) {
    sendPreviousMillis = currentMillis1;
    send2Data();
  }

}


float getTemp() {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized");
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}

int countTrueCommand;
int countTimeCommand;
boolean found = false;
void sendCommand(String command, int maxTime, char readReplay[]) {
  Serial.print(countTrueCommand);
  Serial.print(". at command => ");
  Serial.print(command);
  Serial.print(" ");
  while (countTimeCommand < (maxTime * 1))
  {
    esp.println(command);//at+cipsend
    if (esp.find(readReplay)) //ok
    {
      found = true;
      break;
    }

    countTimeCommand++;
  }

  if (found == true)
  {
    Serial.println("OYI");
    countTrueCommand++;
    countTimeCommand = 0;
  }

  if (found == false)
  {
    Serial.println("Fail");
    countTrueCommand = 0;
    countTimeCommand = 0;
  }

  found = false;
}


boolean send2Data() {
  String getData = "GET /update?apiKey=" + String(API);
  getData += "&field1=" + String(field1_data);

  Serial.println(getData);

  sendCommand("AT+CIPMUX=1", 5, "OK");
  sendCommand("AT+CIPSTART=0,\"TCP\",\"" + HOST + "\"," + PORT, 15, "OK");
  sendCommand("AT+CIPSEND=0," + String(getData.length() + 4), 4, ">");
  esp.println(getData);
  delay(1500);
  countTrueCommand++;
  sendCommand("AT+CIPCLOSE=0", 5, "OK");
}

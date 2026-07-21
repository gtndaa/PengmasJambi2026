#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BH1750.h>
#include <ArduinoJson.h>
#include "RTClib.h"
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <string.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "secrets.h"   // Berisi THINGNAME, WIFI_SSID, WIFI_PASSWORD, AWS_IOT_ENDPOINT, dan sertifikat

#define FSB12 &FreeSerifBold12pt7b

#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define ANEMOMETER_PIN 27
#define VANE_PIN A0
#define RAIN_PIN 33
#define soilMoisturePin A3
#define CALC_INTERVAL 1000

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define voltagePin A6
#define MAX_DATA 1440
#define UPDATE_BUTTON_PIN 16

WiFiClientSecure net;
PubSubClient client(net);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
ADSWeatherV2 weatherStation(RAIN_PIN, VANE_PIN, ANEMOMETER_PIN);
Adafruit_BME280 bme;
BH1750 lightMeter;
RTC_DS3231 rtc;
TFT_eSPI tft = TFT_eSPI();

float rainAmmount;
float windSpeed;
float windDirection;
float windGust;
float tempRain = 0;
int soilMoistureValue = 0;
const int AirValue = 2700;
const int WaterValue = 1320;
float moisturePercentage = 0;
float tempBME;
float humBME;
float pressBME;
float lux;
float solarRadiation;
float analogValue = 0;
float voltage = 0;
String date;
String timeRTC;
char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int update_mode = 2;                // 0: 30 menit, 1: 15 menit, 2: 2 menit
volatile bool isUpdate = false;
String data_store[MAX_DATA];
String temp[14];
int StringCount = 0;
int countSaveData = 0;
String dataMessage;
String lastData;
uint32_t totalLineData = 0;

unsigned long previousMillis = 0;
unsigned long interval = 30000;

volatile bool isDisplay = false;
volatile bool isReconnectingDisplay = false;
volatile bool manualUpdate = false;
unsigned long lastDisplayTime = 0;
unsigned long displayDelay = 60000;
unsigned long delayPageDisplay = 10000;

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

// ---------- Fungsi WiFi Event ----------
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    isReconnectingDisplay = false;
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    isReconnectingDisplay = true;
}

// ---------- Tombol Manual Update ----------
void triggerManualUpdate() {
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 200) {
        manualUpdate = true;
    }
    last_interrupt_time = interrupt_time;
    Serial.println("Update Data Manually");
}

// ---------- AWS IoT ----------
void connectAWS() {
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    client.setServer(AWS_IOT_ENDPOINT, 8883);
    client.setCallback(messageHandler);

    Serial.printf("\nConnecting to AWS IOT\n");
    unsigned long lastMillis = millis();

    if (WiFi.status() == WL_CONNECTED) {
        while (!client.connect(THINGNAME)) {
            Serial.print(".");
            if ((millis() - lastMillis) > 10000) {
                ESP.restart();
            }
            delay(100);
        }
    }

    if (!client.connected()) {
        Serial.println("AWS IoT Timeout!");
        return;
    }

    client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
    Serial.println("AWS IoT Connected!");
}

void publishMessage() {
    StaticJsonDocument<200> doc;

    doc["timestamp"][0] = date;
    doc["timestamp"][1] = timeRTC;
    doc["voltage"] = voltage;
    doc["BH1750"][0] = lux;
    doc["BH1750"][1] = solarRadiation;
    doc["BME"][0] = humBME;
    doc["BME"][1] = tempBME;
    doc["BME"][2] = pressBME;
    doc["ADS"][0] = windSpeed;
    doc["ADS"][1] = windDirection;
    doc["ADS"][2] = windGust;
    doc["ADS"][3] = rainAmmount;
    // Bagian irigasi dan emergency telah dihapus

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(char* topic, byte* payload, unsigned int length) {
    Serial.print("incoming: ");
    Serial.println(topic);
    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload);
    const char* message = doc["message"];
    Serial.println(message);
}

// ---------- Fungsi SD Card dan Data ----------
String LatestData(fs::FS &fs, const char *path, uint32_t position) {
    File file = fs.open(path);
    String buffer;
    if (file) {
        file.seek(position);
        while (file.available()) {
            buffer = file.readStringUntil('\n');
        }
        file.close();
    }
    return buffer;
}

void splitString(String str, String *strs) {
    StringCount = 0;
    while (str.length() > 0) {
        int index = str.indexOf(',');
        if (index == -1) {
            strs[StringCount++] = str;
            break;
        } else {
            strs[StringCount++] = str.substring(0, index);
            str = str.substring(index + 1);
        }
    }
}

void initSDCard() {
    delay(500);
    if (!SD.begin()) {
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return;
    }
    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Writing file: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

int countLine(fs::FS &fs, const char *path) {
    File file = fs.open(path);
    uint32_t lineStart = 0;
    if (file) {
        while (file.available()) {
            lineStart = file.position();
            if (!file.find((char *)"\n"))
                break;
        }
        file.close();
    }
    return lineStart;
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Appending to file: %s\n", path);
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void deleteFile(fs::FS &fs, const char *path) {
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path)) {
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void saveAllData(fs::FS &fs, const char *path, String* store) {
    File file = fs.open(path);
    String buffer;
    int i = 0;
    if (file) {
        while (file.available()) {
            buffer = file.readStringUntil('\n');
            store[i++] = buffer;
        }
        file.close();
    }
}

int waitTime(int a, int *b, int c) {
    int wait = 0;
    for (int i = 0; i < a; i++) {
        if (c >= b[a - 1]) {
            b[i] = 60;
        }
        if (c - b[i] < 0) {
            wait = abs(c - b[i]);
            break;
        }
    }
    return wait;
}

// ---------- Fungsi Tanggal dan Waktu ----------
String formatDate(int a, int b, int c) {
    String formattedDate;
    String temp_a = String(a);
    String temp_b = String(b);
    String temp_c = String(c);
    if (a < 10) temp_a = "0" + temp_a;
    if (b < 10) temp_b = "0" + temp_b;
    if (c < 10) temp_c = "0" + temp_c;
    formattedDate = temp_c + "-" + temp_b + "-" + temp_a;
    return formattedDate;
}

String formatTime(int a, int b, int c) {
    String formattedTime;
    String temp_a = String(a);
    String temp_b = String(b);
    String temp_c = String(c);
    if (a < 10) temp_a = "0" + temp_a;
    if (b < 10) temp_b = "0" + temp_b;
    if (c < 10) temp_c = "0" + temp_c;
    formattedTime = temp_a + ":" + temp_b + ":" + temp_c;
    return formattedTime;
}

void updateDate() {
    DateTime nowDT = rtc.now();
    date = formatDate(nowDT.day(), nowDT.month(), nowDT.year());
}

void updateTime() {
    DateTime nowDT = rtc.now();
    timeRTC = formatTime(nowDT.hour(), nowDT.minute(), nowDT.second());
}

void showDate() {
    DateTime nowDT = rtc.now();
    Serial.print(daysOfTheWeek[nowDT.dayOfTheWeek()]);
    Serial.print(" ");
    date = formatDate(nowDT.day(), nowDT.month(), nowDT.year());
    Serial.print(date);
    Serial.print(" ==> ");
}

void showTime() {
    DateTime nowDT = rtc.now();
    timeRTC = formatTime(nowDT.hour(), nowDT.minute(), nowDT.second());
    Serial.println(timeRTC);
}

void updateTime(int *time_update, int update_mode) {
    int totalUpdate;
    if (update_mode == 0) {
        totalUpdate = 2;
    } else if (update_mode == 1) {
        totalUpdate = 4;
    } else {
        totalUpdate = 30;
    }
    for (int i = 0; i < totalUpdate; i++) {
        time_update[i] = i * 60 / totalUpdate;
    }
}

int xUpdate(int update_mode) {
    int amtUpdate;
    if (update_mode == 0) {
        amtUpdate = 2;
    } else if (update_mode == 1) {
        amtUpdate = 4;
    } else {
        amtUpdate = 30;
    }
    return amtUpdate;
}

// ---------- Fungsi Sensor ----------
void bme_update() {
    humBME = bme.readHumidity();
    tempBME = bme.readTemperature();
    pressBME = bme.readPressure() / 100.0F;
}

void weather_get_update() {
    rainAmmount = weatherStation.getRain() / 4000 - tempRain;
    windSpeed = weatherStation.getWindSpeed();
    windDirection = weatherStation.getWindDirection();
    windGust = weatherStation.getWindGust();
    tempRain = rainAmmount;
}

void voltage_update() {
    analogValue = analogRead(voltagePin);
    voltage = map(analogValue, 0, 4095, 0.0, 3280.0);
    voltage /= 200;
    if (voltage < 11.9) {
        voltage *= 1.1;
    } else if ((voltage > 11.9) && (voltage < 12.9)) {
        voltage *= 1.08;
    } else if ((voltage > 12.9) && (voltage < 13.9)) {
        voltage *= 1.04;
    } else if ((voltage > 13.9) && (voltage < 14.6)) {
        voltage *= 1.02;
    } else if (voltage > 14.61) {
        voltage *= 0.98;
    }
}

void soilMoisture_update() {
    soilMoistureValue = analogRead(soilMoisturePin);
    moisturePercentage = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
    if (moisturePercentage > 100) {
        moisturePercentage = 100;
    } else if (moisturePercentage < 0) {
        moisturePercentage = 0;
    }
}

void lux_update() {
    lux = lightMeter.readLightLevel();
    solarRadiation = (lux * 0.0079);
}

void serialMonitor() {
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println(" V");
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
    Serial.print("Solar Radiation: ");
    Serial.print(solarRadiation);
    Serial.println(" W/m2");
    Serial.print("Moisture: ");
    Serial.print(moisturePercentage);
    Serial.println("%");
    Serial.print("Humidity: ");
    Serial.print(humBME);
    Serial.println("%");
    Serial.print("Temperature: ");
    Serial.print(tempBME);
    Serial.println("°C");
    Serial.print("Pressure: ");
    Serial.print(pressBME);
    Serial.println("mbar");
    Serial.print("Wind speed: ");
    Serial.print(windSpeed);
    Serial.println(" km/h");
    Serial.print("Gusting at: ");
    Serial.print(windGust);
    Serial.println(" km/h");
    Serial.print("Wind Direction: ");
    Serial.print(windDirection);
    Serial.println("°");
    Serial.print("Total Rain: ");
    Serial.print(rainAmmount);
    Serial.println(" liter");
    Serial.println("-------------------------------");
}

// ---------- Fungsi Tampilan ----------
void header(const char *string) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_MAGENTA, TFT_BLUE);
    tft.fillRect(0, 0, 480, 30, TFT_BLUE);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(string, 239, 2, 4);
}

void tftStartDisplay() {
    int xpos = 50;
    int ypos = 50;
    tft.fillScreen(TFT_NAVY);
    header("Sistem Kalender Pertanian");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(xpos, ypos);
    tft.setFreeFont(FSB12);
    tft.println();
    tft.print("AgriCal");
}

void tftDisplayData1() {
    int xpos = 0;
    int ypos = 40;
    tft.fillScreen(TFT_NAVY);
    header("AgriCal (Data 1)");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(xpos, ypos);
    tft.setFreeFont(FSB12);
    tft.println();
    tft.print("humidity (%): ");
    tft.print(humBME);
    tft.setFreeFont(FSB12);
    tft.println();
    tft.print("Temperature (°C): ");
    tft.print(tempBME);
    tft.setFreeFont(FSB18);
    tft.println();
    tft.print("Pressure (mbar): ");
    tft.print(pressBME);
    tft.setFreeFont(FSB24);
    tft.println();
    tft.print("Lux (lx): ");
    tft.print(lux);
}

void tftDisplayData2() {
    int xpos = 0;
    int ypos = 40;
    tft.fillScreen(TFT_NAVY);
    header("AgriCal (Data 2)");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(xpos, ypos);
    tft.setFreeFont(FSB12);
    tft.println();
    tft.print("windSpeed (km/h): ");
    tft.print(windSpeed);
    tft.setFreeFont(FSB12);
    tft.println();
    tft.print("windDirection (°): ");
    tft.print(windDirection);
    tft.setFreeFont(FSB18);
    tft.println();
    tft.print("windGust (km/h): ");
    tft.print(windGust);
    tft.setFreeFont(FSB24);
    tft.println();
    tft.print("rainAmmount (l): ");
    tft.print(rainAmmount);
}

void tftReconnectDisplay() {
    int xpos = 0;
    int ypos = 40;
    tft.fillScreen(TFT_NAVY);
    header("WiFi not connected");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(xpos, ypos);
    tft.setFreeFont(FSB12);
    tft.println();
    tft.print("Reconnecting...");
}

void oledDisplay() {
    display.clearDisplay();
    display.setTextSize(0.7);
    display.setCursor(0, 0);
    display.print("Light : ");
    display.print(lux);
    display.println(" lx");
    display.setCursor(0, 8);
    display.print("Irr : ");
    display.print(solarRadiation);
    display.println(" W/m^2");
    display.setCursor(0, 16);
    display.print("Moisture: ");
    display.print(moisturePercentage);
    display.println(" %");
    display.setCursor(0, 24);
    display.print("Humidity: ");
    display.print(humBME);
    display.println(" %");
    display.setCursor(0, 32);
    display.print("Temp : ");
    display.print(tempBME);
    display.println(" °C");
    display.setCursor(0, 40);
    display.print("Pressure: ");
    display.print(pressBME);
    display.println("mbar");
    display.setCursor(0, 48);
    display.print("Wind Spd: ");
    display.print(windSpeed);
    display.println(" km/h");
    display.setCursor(0, 56);
    display.print("Voltage : ");
    display.print(voltage);
    display.println(" V");
    display.display();
}

void displayPage() {
    if ((millis() - lastDisplayTime) > displayDelay) {
        isDisplay = false;
    } else if ((millis() - lastDisplayTime) <= 600) {
        tftDisplayData1();
    } else if (((millis() - lastDisplayTime) > delayPageDisplay) &&
               ((millis() - lastDisplayTime) <= delayPageDisplay + 600)) {
        tftDisplayData2();
    } else if (((millis() - lastDisplayTime) > 2 * delayPageDisplay) &&
               ((millis() - lastDisplayTime) <= 2 * delayPageDisplay + 600)) {
        tftDisplayData1();
    } else if (((millis() - lastDisplayTime) > 3 * delayPageDisplay) &&
               ((millis() - lastDisplayTime) <= 3 * delayPageDisplay + 600)) {
        tftDisplayData2();
    } else if (((millis() - lastDisplayTime) > 4 * delayPageDisplay) &&
               ((millis() - lastDisplayTime) <= 4 * delayPageDisplay + 600)) {
        tftDisplayData1();
    } else if (((millis() - lastDisplayTime) > 5 * delayPageDisplay) &&
               ((millis() - lastDisplayTime) <= 5 * delayPageDisplay + 600)) {
        tftDisplayData2();
    }
}

// ---------- SETUP ----------
void setup() {
    Serial.begin(115200);

    pinMode(UPDATE_BUTTON_PIN, INPUT);

    initSDCard();
    File file = SD.open("/data.txt");
    if (!file) {
        Serial.println("File doesn't exist");
        Serial.println("Creating file...");
        writeFile(SD, "/data.txt", "date, time, voltage (V), lux (lx), solarRadiation (W/m^2), moisture (%), humidity (%), temperature (°C), pressure (mbar), windSpeed (km/h), windDirection (°), windGust (km/h), rainAmmount (l) \r\n");
    } else {
        totalLineData = countLine(SD, "/data.txt");
        lastData = LatestData(SD, "/data.txt", totalLineData);
        splitString(lastData, temp);
        voltage = temp[3].toFloat();
        lux = temp[4].toFloat();
        solarRadiation = temp[5].toFloat();
        moisturePercentage = temp[6].toFloat();
        humBME = temp[7].toFloat();
        tempBME = temp[8].toFloat();
        pressBME = temp[9].toFloat();
        windSpeed = temp[10].toFloat();
        windDirection = temp[11].toFloat();
        windGust = temp[12].toFloat();
        rainAmmount = temp[13].toFloat();
        Serial.println("File already exists");
    }
    file.close();

    tft.begin();
    tft.setRotation(1);

    Wire.begin();
    rtc.begin();
    DateTime now = rtc.now();
    DateTime compiled = DateTime(F(__DATE__), F(__TIME__));
    if (now.unixtime() < compiled.unixtime()) {
        Serial.println("RTC is older than compile time! Updating");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    lightMeter.begin();

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    if (!bme.begin(0x76)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }

    // WiFi & AWS setup
    WiFi.disconnect(true);
    delay(1000);
    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("AgriCal_AP", "password");  // Access point opsional
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("Access Point IP address: ");
    Serial.println(myIP);
    delay(1000);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    connectAWS();

    tftStartDisplay();
    display.clearDisplay();
    display.setTextSize(2.5);
    display.setTextColor(WHITE);
    display.setCursor(20, 20);
    display.println("Agrical");
    display.display();
    delay(5000);

    attachInterrupt(digitalPinToInterrupt(RAIN_PIN), weatherStation.countRain, FALLING);
    attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), weatherStation.countAnemometer, FALLING);
    attachInterrupt(digitalPinToInterrupt(UPDATE_BUTTON_PIN), triggerManualUpdate, FALLING);

    Serial.println();
    Serial.println("Program started");
}

// ---------- LOOP ----------
void loop() {
    weatherStation.update();
    updateDate();
    updateTime();

    int amtUpdate = xUpdate(update_mode);
    int* time_update = (int*)malloc(amtUpdate * sizeof(int));
    updateTime(time_update, update_mode);

    DateTime now = rtc.now();
    for (int i = 0; i < amtUpdate; i++) {
        if (now.minute() == time_update[i] && now.second() < 1) {
            isUpdate = true;
            break;
        } else {
            isUpdate = false;
        }
    }

    // Cek koneksi WiFi dan AWS
    if (WiFi.status() == WL_CONNECTED) {
        isReconnectingDisplay = false;
        if (!client.connect(THINGNAME)) {
            connectAWS();
        }

        // Kirim data backup jika ada
        File fileBackup = SD.open("/data_backup.txt");
        if (fileBackup) {
            Serial.println("Send Stored Data to AWS IOT Cloud");
            totalLineData = countLine(SD, "/data_backup.txt");
            lastData = LatestData(SD, "/data_backup.txt", totalLineData);
            splitString(lastData, temp);
            countSaveData = temp[0].toInt();
            saveAllData(SD, "/data_backup.txt", data_store);
            Serial.printf("Send %d data: \n", countSaveData);
            for (int i = 0; i < countSaveData; i++) {
                splitString(data_store[i], temp);
                date = temp[1];
                timeRTC = temp[2];
                voltage = temp[3].toFloat();
                lux = temp[4].toFloat();
                solarRadiation = temp[5].toFloat();
                moisturePercentage = temp[6].toFloat();
                humBME = temp[7].toFloat();
                tempBME = temp[8].toFloat();
                pressBME = temp[9].toFloat();
                windSpeed = temp[10].toFloat();
                windDirection = temp[11].toFloat();
                windGust = temp[12].toFloat();
                rainAmmount = temp[13].toFloat();
                publishMessage();
                delay(1000);
            }
            memset(data_store, 0, sizeof(data_store));
            deleteFile(SD, "/data_backup.txt");
        }
        fileBackup.close();
    } else {
        isReconnectingDisplay = true;
    }

    // Update data jika jadwal tiba atau manual
    if (isUpdate || manualUpdate) {
        manualUpdate = false;
        showDate();
        showTime();

        bme_update();
        weather_get_update();
        voltage_update();
        soilMoisture_update();
        lux_update();
        serialMonitor();

        dataMessage = date + "," + timeRTC + "," + String(voltage) + "," +
                      String(lux) + "," + String(solarRadiation) + "," +
                      String(moisturePercentage) + "," + String(humBME) + "," +
                      String(tempBME) + "," + String(pressBME) + "," +
                      String(windSpeed) + "," + String(windDirection) + "," +
                      String(windGust) + "," + String(rainAmmount) + "\r\n";

        Serial.printf("\nSaving data to SD Card: ");
        Serial.print(dataMessage);
        appendFile(SD, "/data.txt", dataMessage.c_str());

        isDisplay = true;
        lastDisplayTime = millis();

        // Kirim ke AWS jika online, simpan backup jika offline
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Interlock! Save Data to SD Card\n");
            File file = SD.open("/data_backup.txt");
            if (!file) {
                Serial.println("File for backup data doesn't exist");
                Serial.println("Creating file for backup...");
                writeFile(SD, "/data_backup.txt", "");
                countSaveData = 0;
            } else {
                Serial.println("File for backup data already exists");
                totalLineData = countLine(SD, "/data_backup.txt");
                lastData = LatestData(SD, "/data_backup.txt", totalLineData);
                splitString(lastData, temp);
                countSaveData = temp[0].toInt();
            }
            file.close();
            countSaveData += 1;
            String temp_dataMessage = String(countSaveData) + "," + dataMessage;
            appendFile(SD, "/data_backup.txt", temp_dataMessage.c_str());
            Serial.printf("Total Stored Data: %d\n", countSaveData);
        } else {
            publishMessage();
            Serial.println("Send Data to AWS IOT Cloud");
        }
    }

    // Tampilan
    if (isDisplay) {
        oledDisplay();
        displayPage();
    } else if (isReconnectingDisplay) {
        tftReconnectDisplay();
        int wait = waitTime(amtUpdate, time_update, now.minute());
        // Opsional: tampilkan sisa waktu di TFT jika diinginkan
    } else {
        // Matikan layar jika tidak ada tampilan aktif
        tft.fillScreen(TFT_BLACK);
        display.clearDisplay();
        display.display();
    }

    // Log waktu tunggu setiap timerDelay
    if ((millis() - lastTime) > timerDelay) {
        int wait = waitTime(amtUpdate, time_update, now.minute());
        Serial.print("Wait ");
        Serial.print(wait);
        Serial.println(" Minutes Again");
        showTime();
        lastTime = millis();
    }

    free(time_update);
    client.loop();
    delay(500);
}
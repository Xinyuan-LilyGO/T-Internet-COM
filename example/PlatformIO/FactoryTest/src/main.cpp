#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>
#include <SD.h>
#include "Adafruit_NeoPixel.h"
#include <HTTPClient.h>


#define WIFI_SSID ""
#define WIFI_PASS ""



//PIN
#define SD_MISO         2
#define SD_MOSI         15
#define SD_SCLK         14
#define SD_CS           13
/*
   * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
   * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
*/
#define ETH_CLK_MODE    ETH_CLOCK_GPIO0_OUT

// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define ETH_POWER_PIN   4

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE        ETH_PHY_LAN8720

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR        0

// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN     23

// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN    18

#define NRST            5

#define PCIE_PWR 34

#define PCIE_TX 33
#define PCIE_RX 35

#define PCIE_ADC 36
#define PCIE_LED 37

#define PCIE_RST 32

#define LEDS_PIN 12

HardwareSerial SerialAT(2);
Adafruit_NeoPixel pixels(1, LEDS_PIN, NEO_GRB + NEO_KHZ800);

HTTPClient http;
static bool eth_connected = false;
static bool eth_done = false;
static bool wifi_done = false;


void WiFiEvent(WiFiEvent_t event)
{
#if ESP_IDF_VERSION_MAJOR > 3
    switch (event) {
    case ARDUINO_EVENT_ETH_START:
        Serial.println("ETH Started");
        // set eth hostname here
        ETH.setHostname("esp32-ethernet");
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        Serial.println("ETH Connected");
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        Serial.print("ETH MAC: ");
        Serial.print(ETH.macAddress());
        Serial.print(", IPv4: ");
        Serial.print(ETH.localIP());
        if (ETH.fullDuplex()) {
            Serial.print(", FULL_DUPLEX");
        }
        Serial.print(", ");
        Serial.print(ETH.linkSpeed());
        Serial.println("Mbps");
        eth_connected = true;
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        Serial.println("ETH Disconnected");
        eth_connected = false;
        break;
    case ARDUINO_EVENT_ETH_STOP:
        Serial.println("ETH Stopped");
        eth_connected = false;
        break;
    default:
        break;
    }
#elif
    switch (event) {
    case SYSTEM_EVENT_ETH_START:
        Serial.println("ETH Started");
        // set eth hostname here
        ETH.setHostname("esp32-ethernet");
        break;
    case SYSTEM_EVENT_ETH_CONNECTED:
        Serial.println("ETH Connected");
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        Serial.print("ETH MAC: ");
        Serial.print(ETH.macAddress());
        Serial.print(", IPv4: ");
        Serial.print(ETH.localIP());
        if (ETH.fullDuplex()) {
            Serial.print(", FULL_DUPLEX");
        }
        Serial.print(", ");
        Serial.print(ETH.linkSpeed());
        Serial.println("Mbps");
        eth_connected = true;
        break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
        Serial.println("ETH Disconnected");
        eth_connected = false;
        break;
    case SYSTEM_EVENT_ETH_STOP:
        Serial.println("ETH Stopped");
        eth_connected = false;
        break;
    default:
        break;
    }
#endif
}

void testClient(const char *host, uint16_t port)
{
    Serial.print("\nconnecting to ");
    Serial.println(host);

    WiFiClient client;
    if (!client.connect(host, port)) {
        Serial.println("connection failed");
        return;
    }
    client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
    while (client.connected() && !client.available())
        ;
    while (client.available()) {
        Serial.write(client.read());
    }

    Serial.println("closing connection\n");
    client.stop();
}



void PCIEInit(void)
{
    pinMode(PCIE_RST, OUTPUT);
    SerialAT.begin(115200, SERIAL_8N1, PCIE_RX, PCIE_TX);
    digitalWrite(PCIE_RST, 1);
    delay(200);
    digitalWrite(PCIE_RST, 0);
    delay(200);
    SerialAT.println("AT");
}

void SD_test(void)
{
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        Serial.println("SDCard MOUNT FAIL");
    } else {
        uint32_t cardSize = SD.cardSize() / (1024 * 1024);
        String str = "SDCard Size: " + String(cardSize) + "MB";
        Serial.println(str);
    }
}



void PCIE_test(void)
{
    static bool pcie_check;
    uint32_t time = 0;
    if (pcie_check = false) {
        SerialAT.flush();
        do {
            SerialAT.println("AT+CPIN?");
            time++;

        } while (!SerialAT.find("READY"));
    }
}

void WIFI_test(void *pvParameter)
{
    bool isConnected = false;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println("scan start");

    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
        }

        if (!isConnected) {
            WiFi.begin(WIFI_SSID, WIFI_PASS);
        }
        while (WiFi.status() != WL_CONNECTED) {
            vTaskDelay(500);
            Serial.print(".");
        }
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        isConnected = true;
    }

    Serial.println("");

    http.begin("https://www.baidu.com/");
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        Serial.println(http.getString());
    }
    WiFi.disconnect();
    wifi_done = true;

    vTaskDelete(NULL);
}

// Fill pixels pixels one after another with a color. pixels is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// pixels.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait)
{
    for (int i = 0; i < pixels.numPixels(); i++) { // For each pixel in pixels...
        pixels.setPixelColor(i, color);         //  Set pixel's color (in RAM)
        pixels.show();                          //  Update pixels to match
        delay(wait);                           //  Pause for a moment
    }
}

void LED_test(void *pvParameter)
{
    pixels.begin();
    pixels.setBrightness(50);
    while (1) {
        colorWipe(pixels.Color(255,   0,   0), 50);      // Red
        colorWipe(pixels.Color(  0, 255,   0), 50);      // Green
        colorWipe(pixels.Color(  0,   0, 255), 50);      // Blue
        colorWipe(pixels.Color(  0,   0,   0, 255), 50); // True white (not RGB white)
        delay(10);
    }
}

void setup()
{
    Serial.begin(115200);

    // xTaskCreatePinnedToCore(WIFI_test, "Task1", 1024 * 20, NULL, 2, NULL, 0);

    xTaskCreatePinnedToCore(LED_test, "Task2", 1024, NULL, 2, NULL, 0);

    // PCIEInit();

    // SD_test();

    WiFi.onEvent(WiFiEvent);

    pinMode(NRST, OUTPUT);
    digitalWrite(NRST, 0);
    delay(200);
    digitalWrite(NRST, 1);
    delay(200);
    digitalWrite(NRST, 0);
    delay(200);
    digitalWrite(NRST, 1);
    delay(200);

    ETH.begin(ETH_ADDR,
              ETH_POWER_PIN,
              ETH_MDC_PIN,
              ETH_MDIO_PIN,
              ETH_TYPE,
              ETH_CLK_MODE);
}

uint32_t lastMillis;

void loop()
{

    if (eth_connected) {
        if (lastMillis < millis()) {
            testClient("baidu.com", 80);
            lastMillis = millis() + 3000;
        }
    }

    while (Serial.available())
        SerialAT.write(Serial.read());
    while (SerialAT.available())
        Serial.write(SerialAT.read());
}

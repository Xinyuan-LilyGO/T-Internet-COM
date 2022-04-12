#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>
#include <SD.h>
#include "Config/config.h"
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include <HTTPClient.h>

HardwareSerial SerialAT(2);
Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(1, LEDS_PIN, 0, TYPE_GRB);
HTTPClient http;
static bool eth_connected = false;
static bool eth_done = false;
static bool wifi_done = false;

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
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
    if (ETH.fullDuplex())
    {
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
}

void testClient(const char *host, uint16_t port)
{
  Serial.print("\nconnecting to ");
  Serial.println(host);

  WiFiClient client;
  if (!client.connect(host, port))
  {
    Serial.println("connection failed");
    return;
  }
  client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
  while (client.connected() && !client.available())
    ;
  while (client.available())
  {
    Serial.write(client.read());
  }

  Serial.println("closing connection\n");
  client.stop();
}
void LEDtest(void)
{
  for (int j = 0; j < 255; j += 2)
  {
    for (int i = 0; i < 1; i++)
    {
      strip.setLedColorData(i, strip.Wheel((i * 256 / 1 + j) & 255));
    }
    strip.show();
    delay(5);
  }
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
  if (!SD.begin(SD_CS))
  {
    Serial.println("SDCard MOUNT FAIL");
  }
  else
  {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    String str = "SDCard Size: " + String(cardSize) + "MB";
    Serial.println(str);
  }
}

void ETH_test(void)
{
  pinMode(NRST, OUTPUT);
  digitalWrite(NRST, 0);
  delay(200);
  digitalWrite(NRST, 1);
  delay(200);
  digitalWrite(NRST, 0);
  delay(200);
  digitalWrite(NRST, 1);

  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
}

void PCIE_test(void)
{
  static bool pcie_check;
  uint32_t time = 0;
  if (pcie_check = false)
  {
    SerialAT.flush();
    do
    {
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
  if (n == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
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

    if (!isConnected)
    {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
    while (WiFi.status() != WL_CONNECTED)
    {
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
  if (httpCode == HTTP_CODE_OK)
  {
    Serial.println(http.getString());
  }
  WiFi.disconnect();
  wifi_done = true;

  vTaskDelete(NULL);
}

void LED_test(void *pvParameter)
{
  strip.begin();
  strip.setBrightness(10);
  while (1)
  {
    LEDtest();
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

void setup()
{
  Serial.begin(115200);

  pinMode(ETH_POWER_PIN, OUTPUT);
  digitalWrite(ETH_POWER_PIN, 1);

  // xTaskCreatePinnedToCore(WIFI_test, "Task1", 1024 * 20, NULL, 2, NULL, 0);
  // xTaskCreatePinnedToCore(LED_test, "Task2", 1024, NULL, 2, NULL, 0);

  // PCIEInit();
  // SD_test();
  WiFi.onEvent(WiFiEvent);
  ETH_test();
}

void loop()
{

  if (eth_connected && !eth_done)
  {
    testClient("baidu.com", 80);
    eth_done = true;
  }

  while (Serial.available())
    SerialAT.write(Serial.read());
  while (SerialAT.available())
    Serial.write(SerialAT.read());
}

/* Example sketch for measuring temperature and humidity using a DHT11
 * connected to an ESP8266-based WeMos D1 R1 and relaying the data to 
 * a PC running MegunoLink Pro using UDP over WiFi for data logging and 
 * visualisation.
 * Written by Elliot Baptist 17/1/9 for Number Eight Innovation
 * Uses DHT-sensor-library by Adafruit and WiFiManager by tzapu
 */

#include "DNSServer.h" // for redirecting to the configuration portal
#include "ESP8266WebServer.h" // for serving the configuration portal
#include "WiFiManager.h" // WiFi configuration

#include "DHT.h" // Adafruit DHT-sensor-library

#include "MegunoLink.h" // functions for communicating to Megunolink Pro
#include "Filter.h" // Megunolink exponential filter functions

#include "ESP8266WiFi.h" // ESP8266 core WiFi Arduino library
#include "WiFiUdp.h" // UDP communications

// DHT sensor type
#define DHT_TYPE DHT11

// DHT sensor pin definition, used to communicate with the DHT11
#define DHT_DATA_PIN D4 
// Note: pin "4" (ESP8266 GPIO4) 
// is different to pin "D4" (GPIO2 marked as "D4" on the WeMos D1 mini dev board)
 

// UDP connection configuration
#define SOURCE_UDP_PORT 52792 // UDP port to receive from
#define DESTINATION_UDP_PORT 52791 // UDP Port to send to

#define DENSTINATION_IP_ADDRESS 255,255,255,255 
                                // comma seperated IP Address to send measuremets to,
                                // use 255,255,255,255 for all devices on network

// Declare global variables --------------------------------------------------------
// WiFi connection management
WiFiManager wiFiManager; 

// Initialise the DHT11 sensor
DHT dhtSens(DHT_DATA_PIN, DHT_TYPE);  

// Exponential filters for measurements
ExponentialFilter<float> temperatureFilter(5, 25); // filter for temperature
ExponentialFilter<float> humidityFilter(5, 50); // filter for humidity

// UDP connection management
WiFiUDP udp; 

// Destination IP Address to use for UDP
IPAddress destinationIp(DENSTINATION_IP_ADDRESS); 

// Keep track of measurement transmissions,
// will overflow after 4,294,967,295 measurements
unsigned long transmitCount = 0; 

// setup ---------------------------------------------------------------------- setup
void setup()
{
  // Open debug serial connection
  Serial.begin(9600);  
  Serial.println("");
  Serial.println("DHT11+ESP8266 WiFi UDP to MegunoLink Data Logger");

  // Configure built in LED pin as output so it can flash as a 'heartbeat'
  pinMode(LED_BUILTIN, OUTPUT);

  // Start sensor communication library
  dhtSens.begin(); 

  // Connect to WiFi. Will start a captive portal access point for configuration 
  // if cannot connect to the last saved connection or is first time setup.
  // To access configuration portal, connect to ESP<chip number here> over WiFi
  // with phone/PC/tablet
  Serial.print("Attempting to connect to WiFi: ");
  wiFiManager.autoConnect();
  Serial.println("connected!");

  // Start WiFi UDP communication library
  udp.begin(SOURCE_UDP_PORT);
  
  // Done
  Serial.println("Startup complete, debug serial stream:");
}

// loop ------------------------------------------------------------------------ loop
void loop() 
{ 
  // Read sensor
  float temperature = dhtSens.readTemperature(); // in degrees celsius
  float humidity = dhtSens.readHumidity(); // in percent relative humidity

  // Debug serial output of raw tab seperated values
  Serial.print("Temperature [degC]\t");
  Serial.print(temperature);
  Serial.print("\tHumidity [%RH]\t");
  Serial.print(humidity);
  Serial.println("");

  // Add values to filters if they are valid
  if(!isnan(temperature) && !isnan(humidity))
  {
    temperatureFilter.Filter(temperature);
    humidityFilter.Filter(humidity);
  }

  // Start assembling UDP packet
  udp.beginPacket(destinationIp, DESTINATION_UDP_PORT);

  TimePlot Plot("DHT", udp);
 
  // Add plot display information every so often for late listeners
  if (transmitCount % 10 == 0)
  {
    Plot.SetTitle("Remote Temperature & Humidity");
    Plot.SetXlabel("Time");
    Plot.SetYlabel("Degrees C / % Relative Humidity");
    Plot.SetSeriesProperties("Raw Temperature", TimePlot::Blue, TimePlot::NoLine, 1, TimePlot::Square);
    Plot.SetSeriesProperties("Raw Humidity", TimePlot::Red, TimePlot::NoLine, 1, TimePlot::Circle);
    Plot.SetSeriesProperties("Filtered Temperature", TimePlot::Blue, TimePlot::Solid, 2, TimePlot::NoMarker);
    Plot.SetSeriesProperties("Filtered Humidity", TimePlot::Red, TimePlot::Solid, 2, TimePlot::NoMarker);
  }

  // Add raw measurements
  Plot.SendData("Raw Temperature", temperature);
  Plot.SendData("Raw Humidity", humidity);
  Plot.SendData("Filtered Temperature", temperatureFilter.Current());
  Plot.SendData("Filtered Humidity", humidityFilter.Current());

  // Send and increment counter
  udp.endPacket();
  transmitCount++;

  // Wait a little while before making next measurment
  delay(2000); // delay in milliseconds
}


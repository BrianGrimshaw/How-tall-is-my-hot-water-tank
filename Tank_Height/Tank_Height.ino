// 2025-07-11 Brian Grimshaw
// Please enter your sensitive data in the Secret tab/arduino_secrets.h

#include "Arduino_LED_Matrix.h"

#define CYCLE_TIME         50     // Sample time for reading analogue
#define SAMPLE_TIME      5000     // Sample time (ms) to average analogue readings and for measring drop when tap first opened
#define DROP_SIZE        1000     // Drop value to look for when tap first opened
#define DROP_SAMPLES        3     // Look at previous samples to measure drop
#define DIFF_TAP_CLOSED    50     // Tank gets more than this much taller wwhen tap closes
#define STABLE_TOLERANCE   20     // Tolerance for stable after tap first opened
#define STABLE_SAMPLES      5     // Check 5 samples for stable
#define RUNNING_SAMPLES     3     // Check 3 samples for tap running
#define MAX_SAMPLES        40     // Must have completed by this many samples
#define EMPTY_OFFSET     2000     // Approx 1mm
#define LOG_COUNT          50     // Log this many values each time

#include "WiFiS3.h"

#include "arduino_secrets.h" 
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(80);

int state = 0;                    // 0 = heated, 1 = tap opened, 2 = wait for stable, 3 = reading
int full = 15000; // Full tank value - all hot
int prevFull[] = {0,0,0,0,0}; // 
long fullAverage = 0;
int empty = full - EMPTY_OFFSET;  // Empty tank value - all cold
int sampleCount = 0;    // Counter for averaging samples
int sampleTotal = 0;    // Contains total for average
int reading = 0;        // Averaged reading
int prevReading[] = {0,0,0,0,0,0,0,0,0,0};   // No previous readings yet
int prevReadingSize = sizeof(prevReading) / sizeof(prevReading[0]);
bool startRecording = false;
int dropDiff = 0;              // Difference to detect first drop

int diff = 0;                  // Difference to detect first tap opened
bool waterRunning = false;
bool stable = false;
int stableAverage = 0;
int samples = 0;
int percentHot = 0;
unsigned long currentTime = 0;
unsigned long lastTime = 0;
int logIndex = 0;
int logCount = 0;
int logBuffer[1000];
int logBufferSize = sizeof(logBuffer) / sizeof(logBuffer[0]);

ArduinoLEDMatrix matrix;

// Static part of LED matrix
uint8_t frame[8][12] =
{
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
  { 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0 },
  { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }
};

void setup()
{
  analogReadResolution(14); //change to 10-bit resolution (default anyway)
  analogReference(AR_DEFAULT); // Default reference of 5 V
  Serial.begin(9600);
  matrix.begin();

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status

  // Take booted as 100% tank and state 3
  // First reading is too high, so discard it
  int r = analogRead(A1);
  delay(2000);
  // Get an average first reading
  for (int i = 0; i < 10; i++)
  {
    sampleTotal += analogRead(A1); // returns a value between 0-4096 for 0.0 to 5.0V
    delay(50);
  }
  full = sampleTotal / 10;
  reading = full;
  // Populate buffer
  for (int i = 0; i < prevReadingSize; i++)
    prevReading[i] = full;
  
  sampleTotal = 0;
  diff = 0;
  state = 0;
  lastTime = millis();

  // Clear log
  memset(&logBuffer, 0, sizeof(logBuffer));
}

void loop()
{
  // Strings for display on web page
  char readingStr[10];
  char diffStr[10];
  char emptyStr[10];
  char fullStr[10];
  char stateStr[10];
  char hotStr[10];
  char logStr[10];
  
  // Bargraph is 0 for 10% or less, 1 for 20%, 9 = 100%
  int readingRaw = analogRead(A1); // returns a value between 0-4096 for 0.0 to 5.0V
  currentTime = millis();
  if (((unsigned long)(currentTime - lastTime) > CYCLE_TIME) && readingRaw)
  {
    lastTime = currentTime;
    // Work out where 100% is
    // Sample the analogue reading and make an average
    sampleCount++;
    sampleTotal += readingRaw;
    if (sampleCount >= (SAMPLE_TIME / CYCLE_TIME))
    {
      reading = sampleTotal / sampleCount;
      sampleCount = 0;
      sampleTotal = 0;

      // Shift up the array of previous readings
      for (int i = (sizeof(prevReading) / sizeof(prevReading[0])) - 1; i > 0; i--)
      {
        prevReading[i] = prevReading[i - 1];
      }
      prevReading[0] = reading;

      // Log data
      if (startRecording && (logIndex < logBufferSize) && (logCount < LOG_COUNT))
      {
        logBuffer[logIndex] = reading;
        logCount++;
        logIndex++;
      }
      else if ((logIndex >= logBufferSize) || (logCount >= LOG_COUNT))
      {
        startRecording = false;
        logCount = 0;
        logBuffer[logIndex] = 0xFFFF;
        logIndex++;
        Serial.println("Recording stopped");
      }
      // Difference to check
      dropDiff = prevReading[DROP_SAMPLES] - reading;
      if ((dropDiff > DROP_SIZE) && !startRecording)
      {
        startRecording = true;
        Serial.println("Recording started");
        // Put the buffered data in the log oldest first
        int j = 0;
        for (int i = prevReadingSize - 1; i >= 0; i--)
        {
          logBuffer[logIndex + j++] = prevReading[i];
        }
        logIndex += prevReadingSize;
        logCount = prevReadingSize;
      }
      // Look for 3 negatives (or small positive) in a row after drop to show that water is flowing
      // Note: three negatives between four readings
      if (logCount >= (prevReadingSize + 3)) waterRunning = 1;
      for (int i = 1; i <= RUNNING_SAMPLES; i++)
      {
        if ((prevReading[i] - prevReading[i - 1]) > STABLE_TOLERANCE) waterRunning = 0;
      }
      // Stable when five readings in a row where difference is small
      stable = 1;
      long ave = 0;
      for (int i = 1; i < STABLE_SAMPLES; i++)
      {
        if (abs(prevReading[i] - prevReading[i - 1]) > STABLE_TOLERANCE) stable = 0;
        ave += prevReading[i];
      }
      // Need to add the first one as the loop stops one short
      ave += prevReading[0];
      stableAverage = (int)(ave / STABLE_SAMPLES);
      // Difference to check for tap closing
      diff = prevReading[0] - prevReading[2];
      switch (state)
      {
        case 0: // Power on - wait for drop when tap first opened
          frame[7][4] = 0;
          frame[7][5] = 0;
          frame[7][6] = 0;
          frame[7][7] = 0;
          if (startRecording) // Drop detected when recoring started
          {
            state = 1;
            logBuffer[logIndex] = -state;
            logIndex++;
            samples = 0;
          }
          break;
        case 1: // Look for 3 negatives in a row
          frame[7][4] = 1;
          frame[7][5] = 0;
          frame[7][6] = 0;
          frame[7][7] = 0;
          samples++;
          if (waterRunning)
          {
            state = 2;
            logBuffer[logIndex] = -state;
            logIndex++;
          }
          else if (samples > MAX_SAMPLES)
          {
            state = 4;
            logBuffer[logIndex] = -state - 10;
            logIndex++;
          }
          break;
        case 2: // Check for difference to swing positive when tap closed
          frame[7][4] = 1;
          frame[7][5] = 1;
          frame[7][6] = 0;
          frame[7][7] = 0;
          samples++;
          if (diff >= DIFF_TAP_CLOSED) // Tap closed, go to wait until stable
          {
            state = 3;
            logBuffer[logIndex] = -state;
            logIndex++;
          }
          else if (samples > MAX_SAMPLES)
          {
            state = 4;
            logBuffer[logIndex] = -state - 20;
            logIndex++;
          }
          break;
        case 3: // Wait until stable
          frame[7][4] = 1;
          frame[7][5] = 1;
          frame[7][6] = 1;
          frame[7][7] = 0;
          samples++;
          if (stable) // Stable readings now, call it full
          {
            state = 4;
            logBuffer[logIndex] = -state;
            logIndex++;

            // Shift up the array of previous fulls and average them on the way
            fullAverage = 0;
            for (int i = (sizeof(prevFull) / sizeof(prevFull[0])) - 1; i > 0; i--)
            {
              prevFull[i] = (prevFull[i - 1])? prevFull[i - 1] : stableAverage;
              fullAverage += (long)prevFull[i];
            }
            prevFull[0] = stableAverage;
            fullAverage += (long)prevFull[0];
            // Average full over a few fulls
            full = (int)(fullAverage / (sizeof(prevFull) / sizeof(prevFull[0])));         // Full tank value - all hot
            empty = full - EMPTY_OFFSET;  // Empty tank value - all cold
            logBuffer[logIndex] = -full;
            logIndex++;
            logBuffer[logIndex] = -empty;
            logIndex++;
          }
          else if (samples > MAX_SAMPLES)
          {
            state = 4;
            logBuffer[logIndex] = -state - 30;
            logIndex++;
          }
          break;
        case 4: // Tank is now full, wait for recording to stop
          frame[7][4] = 1;
          frame[7][5] = 1;
          frame[7][6] = 1;
          frame[7][7] = 1;
          if (!startRecording) // Wait for recording to stop
          {
            state = 5;
          }
          break;
        case 5: // Tank is now full, wait for drop
          frame[7][4] = 1;
          frame[7][5] = 1;
          frame[7][6] = 1;
          frame[7][7] = 1;
          if (startRecording) // Drop detected when recoring started
          {
            state = 1;
            logBuffer[logIndex] = -state;
            logIndex++;
            samples = 0;
          }
          break;
        default: // Start again
          state = 0;
          break;
      }
      empty = full - EMPTY_OFFSET;

      // Send reading to terminal window
      itoa(reading, readingStr, 10);
      itoa(diff, diffStr, 10);
      itoa(empty, emptyStr, 10);
      itoa(full, fullStr, 10);
      itoa(state, stateStr, 10);
      itoa(percentHot, hotStr, 10);
      Serial.write(readingStr);
      Serial.write("...");
      Serial.write(diffStr);
      Serial.write("...");
      Serial.write(emptyStr);
      Serial.write("...");
      Serial.write(fullStr);
      Serial.write("...");
      Serial.write(stateStr);
      Serial.write("...");
      Serial.write(hotStr);
      if (startRecording)
        Serial.write(" R");
      Serial.write("\n");
      matrix.renderBitmap(frame, 8, 12);
    }
    

    // Work out percentage hot and bargraph
    // y = 0.3003x - 100 from defaults - thanks Excel
    float m = (100.0 - 0.0) / (float)(full - empty);
    float c = 0.0 - (m * (float)empty);
    percentHot = (int)((m * (float)reading) + c);
    int bargraph = (percentHot * 10) / 100;
    if (bargraph < 0) bargraph = 0;
    if (bargraph > 9) bargraph = 9;
    // Display bargraph
    for (int i = 0; i < 9; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        frame[j][i + 2] = i < bargraph;
        frame[j][i + 2] = i < bargraph;
        frame[j][i + 2] = i < bargraph;
      }
    }
  }

  WiFiClient client = server.available();   // listen for incoming clients

  if (client)                               // if you get a client,
  {
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    String httpNewFullStr = "";
    int newFullValue = 0;
    while (client.connected())              // loop while the client's connected
    {
      if (client.available())               // if there's bytes to read from the client,
      {
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor
        httpNewFullStr += c;
        if (c == '\n')                      // if the byte is a newline character
        {
          Serial.println(httpNewFullStr);                    // print it out to the serial monitor
//          newFullValue = atoi(httpNewFullStr);
          if (0)//(newFullValue)
         {
            // New full value received, store it
            for (int i = 0; i < (sizeof(prevFull) / sizeof(prevFull[0])); i++)
            {
              prevFull[i] = newFullValue;
            }
            fullAverage = newFullValue;
          }
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            itoa(reading, readingStr, 10);
            itoa(diff, diffStr, 10);
            itoa(empty, emptyStr, 10);
            itoa(full, fullStr, 10);
            itoa(state, stateStr, 10);
            itoa(percentHot, hotStr, 10);
            client.print("<p>");
            client.print(readingStr);
            client.print("...");
            client.print(diffStr);
            client.print("...");
            client.print(emptyStr);
            client.print("...");
            client.print(fullStr);
            client.print("...");
            client.print(stateStr);
            client.print("...");
            client.print(hotStr);
            // Send log
            for (int j = 0; j < logIndex; j++)
            {
              if (logBuffer[j] == 0xFFFF)
              {
                client.print("<p>");
                client.print("Recording stopped");
              }
              else
              {
                itoa(logBuffer[j], logStr, 10);
                client.print("<p>");
                client.print(logStr);
              }
              client.print("</p>");
            }
            // Start log again
            logIndex = 0;
            memset(&logBuffer, 0, sizeof(logBuffer));
            client.print("</p>");
            
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else     // if you got a newline, then clear currentLine:
          {
            currentLine = "";
          }
        }
        else if (c != '\r')   // if you got anything else but a carriage return character,
        {
          currentLine += c;      // add it to the end of the currentLine
        }
      }
      
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }

}
void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

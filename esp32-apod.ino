#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TJpg_Decoder.h>
#include <LittleFS.h>

const char* ssid = "SSID";
const char* password = "PASSWD";

// Hardware SPI pins
#define TFT_CS   5
#define TFT_DC   21
#define TFT_RST  22

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// TJpg_Decoder callback for rendering JPEG blocks to the display
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return 0;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return 1;
}

void fetchAndDisplayAPOD();
bool downloadFile(const char* url, const char* filename);

void setup() {
    Serial.begin(115200);

    tft.begin();
    tft.setRotation(1); // Landscape orientation (320x240)
    
    // Invert display colors if necessary (uncomment if colors are still wrong)
    // tft.invertDisplay(true); 

    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    
    tft.setCursor(10, 10);
    tft.println("Mounting LittleFS...");

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed!");
        tft.println("FS Error!");
        return;
    }

    tft.println("Connecting WiFi...");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    
    // Set to false to fix the purple/green noise issue on ILI9341
    TJpgDec.setSwapBytes(false); 
    TJpgDec.setCallback(tft_output);
    
    fetchAndDisplayAPOD();
}

void loop() {
    delay(86400000); // Check every 24 hours
    fetchAndDisplayAPOD();
}

void fetchAndDisplayAPOD() {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(10, 10);
    tft.println("Fetching APOD API...");
    
    String apiUrl = "https://science.nasa.gov/wp-json/wp/v2/apod-basic";
    
    WiFiClientSecure client;
    client.setInsecure();
    
    HTTPClient http;
    http.begin(client, apiUrl);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        // Save JSON to disk due to Chunked Transfer Encoding
        File jsonFile = LittleFS.open("/apod.json", "w");
        if (jsonFile) {
            http.writeToStream(&jsonFile);
            jsonFile.close();
        }
        http.end();
        
        File readFile = LittleFS.open("/apod.json", "r");
        JsonDocument filter;
        filter[0]["title"] = true;
        filter[0]["hdurl"] = true;
        filter[0]["url"] = true;
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, readFile, DeserializationOption::Filter(filter));
        readFile.close();
        
        if (!error) {
            String title = doc[0]["title"].as<String>();
            String imageUrl = doc[0]["hdurl"].as<String>(); 
            
            if (imageUrl == "null" || imageUrl.length() == 0) {
                imageUrl = doc[0]["url"].as<String>();
            }
            
            // Modify URL: remove original parameters and force display resolution
            int qIndex = imageUrl.indexOf('?');
            if (qIndex != -1) {
                imageUrl = imageUrl.substring(0, qIndex);
            }
            imageUrl += "?w=320&h=240&fit=clip";
            
            tft.fillScreen(ILI9341_BLACK);
            tft.setCursor(10, 10);
            tft.println("Downloading image:");
            tft.setTextSize(1);
            tft.setCursor(10, 30);
            tft.println(title);
            
            Serial.println("Title: " + title);
            Serial.println("Optimized URL: " + imageUrl);
            
            if (downloadFile(imageUrl.c_str(), "/apod.jpg")) {
                Serial.println("Download complete. Rendering...");
                
                uint16_t w = 0, h = 0;
                TJpgDec.getFsJpgSize(&w, &h, "/apod.jpg", LittleFS);
                
                // Since we download exactly 320x240, scale is 1 (no scaling)
                TJpgDec.setJpgScale(1);
                
                // Center the image in case the aspect ratio doesn't match perfectly
                int x = (320 - w) / 2;
                int y = (240 - h) / 2;
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                
                tft.fillScreen(ILI9341_BLACK);
                TJpgDec.drawFsJpg(x, y, "/apod.jpg", LittleFS);
                
                // Top bar with title over the image
                tft.fillRect(0, 0, 320, 15, ILI9341_BLACK);
                tft.setCursor(5, 4);
                tft.setTextColor(ILI9341_WHITE);
                tft.println(title);
                
            } else {
                tft.fillScreen(ILI9341_BLACK);
                tft.setCursor(10, 10);
                tft.setTextSize(2);
                tft.println("Failed to download\nimage.");
            }
        } else {
            Serial.print("JSON Error: ");
            Serial.println(error.c_str());
        }
    } else {
        Serial.printf("API Request failed, HTTP Code: %d\n", httpCode);
    }
}

bool downloadFile(const char* url, const char* filename) {
    WiFiClientSecure client;
    client.setInsecure(); 
    
    HTTPClient http;
    http.begin(client, url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        File f = LittleFS.open(filename, "w");
        if (f) {
            http.writeToStream(&f);
            f.close();
            http.end();
            return true;
        }
    }
    http.end();
    return false;
}
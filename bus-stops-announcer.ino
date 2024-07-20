#define DEBUGGING

#define BUTTON_PIN A9
#define NUMBER_OF_BUTTONS 3

#include <AnalogKey.h>
#include <EncButton.h>
#include <SoftwareSerial.h>
#include <DFMiniMp3.h> // https://github.com/Makuna/DFMiniMp3
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include "./display_7_segment.h"
#include "./coordinates.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// #define RX_GPS 15 // arduino rx pin for gps (gps's tx)
// #define TX_GPS 14 // arduino tx pin for gps (gps's rx)
#define GPSBaud 9600 // Default baud of NEO-6M is 9600
// moved to mega with it's own hardware serial so there is no need for software serial
// SoftwareSerial Serial3(RX_GPS, TX_GPS); // Create a software serial port called "Serial3"
TinyGPSPlus gps;
#define EARTH_RADIUS 6371  // Earth's radius in kilometers

int16_t sigs[NUMBER_OF_BUTTONS] = {413, 122, 0}; // array of signal value of buttons
AnalogKey<BUTTON_PIN, NUMBER_OF_BUTTONS, sigs> keys; // pin of button, number of buttons, array of signal
// Button btn(BUTTON_PIN);
VirtButton btn_1;
VirtButton btn_2;
VirtButton btn_3;


#define STARTING_VOLUME 20
class Mp3Notify; // forward declare the notify class, just the name

typedef DFMiniMp3<HardwareSerial, Mp3Notify> DfMp3; // define a handy type using serial and our notify class
DfMp3 dfmp3(Serial2); // instance a DfMp3 object

// Some arduino boards only have one hardware serial port, so a software serial port is needed instead.
// comment out the above definitions and use these
// SoftwareSerial secondarySerial(17, 16); // RX (DF player's TX), TX (DF player's RX)
// typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;
// DfMp3 dfmp3(secondarySerial);

bool increaseRadiusDistanceCheck = true;
uint16_t radiusDistanceCheck = 20;
uint16_t volume = STARTING_VOLUME;
uint8_t audio_number;
bool playStop = false;
uint16_t trackNumber = 1;
uint16_t trackCountInFolder1;
bool secondFinishCall = false;
bool gpsInfoBigText = false;
bool showDistancesInsteadOfCord = true;
double distances[STOPS_MAX] = {0};
uint8_t index_of_shortest[STOPS_MAX];
uint8_t lastStop = 255;
uint64_t stopPassedTime = 0;
uint64_t GPSInfoORDistanceCheckTimer = 0;
bool showGPSInfo = true;

bool startToEnd = true;
uint8_t lastClosestStop = 0;
uint8_t adjacentToLastClosestStop = 1;
double lastDistanceToClosestStop = 0;
double lastDistanceAdjacentToClosestStop = 0;

Coordinates coordinates;

// implement a notification class,
// its member methods will get called
class Mp3Notify {
    public:
        static void PrintlnSourceAction(DfMp3_PlaySources source, const char *action) {
            if (source & DfMp3_PlaySources_Sd) {
                Serial.print("SD Card, ");
            }

            if (source & DfMp3_PlaySources_Usb) {
                Serial.print("USB Disk, ");
            }

            if (source & DfMp3_PlaySources_Flash) {
                Serial.print("Flash, ");
            }

            Serial.println(action);
        }

        static void OnError([[maybe_unused]] DfMp3 &mp3, uint16_t errorCode) {
            // see DfMp3_Error for code meaning
            Serial.println();
            Serial.print("Com Error ");
            Serial.println(errorCode);
        }

        static void OnPlayFinished([[maybe_unused]] DfMp3 &mp3, [[maybe_unused]] DfMp3_PlaySources source, uint16_t track) {
            // stopPassedTime = millis();
            if(playStop) {
                dfmp3.playFolderTrack16(coordinates.currentLine(), audio_number);
                playStop = false;
            }
        }

        static void OnPlaySourceOnline([[maybe_unused]] DfMp3 &mp3, DfMp3_PlaySources source) {
            PrintlnSourceAction(source, "online");
        }

        static void OnPlaySourceInserted([[maybe_unused]] DfMp3 &mp3, DfMp3_PlaySources source) {
            PrintlnSourceAction(source, "inserted");
        }

        static void OnPlaySourceRemoved([[maybe_unused]] DfMp3 &mp3, DfMp3_PlaySources source) {
            PrintlnSourceAction(source, "removed");
        }
};

void setup() {
    keys.setWindow(50);

    Serial.begin(115200);
    
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
    }

    #ifdef DEBUGGING
        display.display();
        delay(700); 
    #endif

    // Clear the buffer
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font

    #ifdef DEBUGGING
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("initializing DF player...");
        display.display();
        Serial.println("initializing DF player...");
    #endif

    dfmp3.begin();
    // for boards that support hardware arbitrary pins
    // dfmp3.begin(6, 5); // RX, TX

    // during development, it's a good practice to put the module
    // into a known state by calling reset().
    // You may hear popping when starting and you can remove this
    // call to reset() once your project is finalized
    dfmp3.reset();

    uint16_t version = dfmp3.getSoftwareVersion();
    uint16_t total_track_count = dfmp3.getTotalTrackCount(DfMp3_PlaySource_Sd);
    trackCountInFolder1 = dfmp3.getFolderTrackCount(1);
    dfmp3.setVolume(STARTING_VOLUME);

    // show some info on dfplayer (to let know that it's working)
    #ifdef DEBUGGING
        Serial.println("done initializing DFPlayer...");
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("done initializing DFPlayer...");
        display.display();
        delay(700);

        Serial.print("version ");
        Serial.println(version);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("version ");
        display.print(version);

        Serial.print("files total ");
        Serial.println(total_track_count);
        display.setCursor(0, 8);
        display.print("tracks total: ");
        display.print(total_track_count);
        display.display();

        Serial.print("volume ");
        Serial.println(volume);
        display.setCursor(0, 16);
        display.print("volume: ");
        display.print(volume);
        display.display();

        delay(700);
    #endif
    // uint16_t volume = dfmp3.getVolume();
    

    // Start the software serial port at the GPS's default baud
    Serial3.begin(GPSBaud);

    pinMode(RESET_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    resetNumber();
    showNumber(coordinates.currentLine());
}

void loop() {
    btn_1.tick(keys.status(0));
    btn_2.tick(keys.status(1));
    btn_3.tick(keys.status(3));

    dfmp3.loop();

    if(btn_1.hasClicks(1)) {
        coordinates.prevLine();
        showNumber(coordinates.currentLine());
    }
    if(btn_3.hasClicks(1)) {
        coordinates.nextLine();
        showNumber(coordinates.currentLine());
    }

    if(btn_2.hasClicks(3)) {
        dfmp3.playFolderTrack16(1, 99); // in folder '01', audio '0099' which says 'audio check'
    }

    if(btn_2.hasClicks(1)) {
        Serial.println("button clicked");
        if(showDistancesInsteadOfCord) {
            gpsInfoBigText = true;
            showDistancesInsteadOfCord = false;
        } else if(gpsInfoBigText) {
            gpsInfoBigText = false;
        } else {
            showDistancesInsteadOfCord = true;
        }
    }

    if(btn_2.step()) {
        Serial.println("button step");
        if(increaseRadiusDistanceCheck) {
            if(radiusDistanceCheck < 30) {
                radiusDistanceCheck++;
                showGPSInfo = false;
                GPSInfoORDistanceCheckTimer = millis();
            }
        } else {
            if(radiusDistanceCheck > 1) {
                radiusDistanceCheck--;
                showGPSInfo = false;
                GPSInfoORDistanceCheckTimer = millis();
            }
        }
    }

    if(btn_2.releaseStep()) {
        Serial.println("button release step");
        increaseRadiusDistanceCheck = !increaseRadiusDistanceCheck;
    }

    if(showGPSInfo) {
        while (Serial3.available() > 0) {
            if (gps.encode(Serial3.read())) {
                Serial.println("\nThe distances are: ");

                // calculate distances to stops
                for (uint8_t i = 0; i < coordinates.getStopsNum(); i++) {
                    distances[i] = calculateDistance(51.24017209067963, 33.21328523813368, coordinates.getLat(i, startToEnd), coordinates.getLng(i, startToEnd)); // for testing
                    // distances[i] = calculateDistance(gps.location.lat(), gps.location.lng(), coordinates.getLat(i, startToEnd), coordinates.getLng(i, startToEnd));

                    index_of_shortest[i] = i;

                    Serial.print(distances[i]);
                    Serial.print("  ");
                }

                // sort distances and their coordinates from smallest to largest
                for (int i = 0; i < coordinates.getStopsNum() - 1; i++) {
                    int minIndex = i;
                    for (int j = i + 1; j < coordinates.getStopsNum(); j++) {
                        if (distances[j] < distances[minIndex]) {
                            minIndex = j;
                        }
                    }
                    if (minIndex != i) {
                        // Swap distances and corresponding indexes
                        swap(distances[i], distances[minIndex]);
                        swap(index_of_shortest[i], index_of_shortest[minIndex]);
                    }
                }

                determineDirection();

                // if within range of a specific stop and not moving then announce the stop
                if(distances[0] <= radiusDistanceCheck && gps.speed.kmph() <= 1) {
                    // if(millis() - stopPassedTime >= 10000) {
                    if(lastStop != index_of_shortest[0]) {
                        lastStop = index_of_shortest[0];
                        // stopPassedTime = millis();

                        // if going back (passed half of the stops), then audio number resets from the half (i.e. index of the stop minus half), else just stop index
                        audio_number = index_of_shortest[0] > coordinates.getStopsNum()/2 ? index_of_shortest[0] - coordinates.getStopsNum()/2 : index_of_shortest[0];
                        audio_number++; // in folder audios start from 1, not 0
                        playStop = true;
                        dfmp3.playFolderTrack16(1, 100); // play audio that says 'stop...'
                    }
                }

                displayInfo();
            }
        }
    } else {
        displayInfo();
    }

}

void displayInfo() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    if(showGPSInfo) {
        Serial.println("showing gps info");
        
        // if(millis() - GPSInfoORDistanceCheckTimer >= 3000) {
        //     showGPSInfo = false;
        //     GPSInfoORDistanceCheckTimer = millis();
        // }

        if (gps.location.isValid()) {
            if(showDistancesInsteadOfCord) { // show six shortest distances
                if(distances[0] <= 40) { // if very close to a stop, then show the distance to it and speed with big text size
                    display.setTextSize(2);
                    display.setCursor(0, 0);
                    display.print(index_of_shortest[0]);
                    display.print(") ");
                    display.print(distances[0]);
                    display.print("m");
                    display.setCursor(0, 17);
                    display.print(gps.speed.kmph());
                    display.print("km/h");

                    // section below doesn't print "!!" because in the main loop when announcing the stop the code updates lastStop
                    // if(distances[0] <= radiusDistanceCheck && gps.speed.kmph() <= 1) {
                    //     if (lastStop != index_of_shortest[0]) {
                    //         display.setCursor(95, 17);
                    //         display.print("!!");
                    //     }
                    // }
                    display.setCursor(105, 17);
                    if(startToEnd) {
                        display.print(">");
                    } else {
                        display.print("<");
                    }
                } else {
                    // print first six 
                    display.setTextSize(1);
                    for(byte i = 0; i < 2; i++) {
                        for (byte j = 0; j < 3; j++) {
                            byte cordNumber = j+(i*3);
                            if(cordNumber == 5) continue; // don't prit sixth coordinated to leave space for movement direction
                            display.setCursor(i * 64, j * 10);
                            display.print(index_of_shortest[cordNumber]);
                            display.print(")");
                            display.print(distances[cordNumber], 1);
                        }
                    }

                    display.setCursor(100, 24);
                    if(startToEnd) {
                        display.print("->");
                    } else {
                        display.print("<-");
                    }
                }
                
            } else if(gpsInfoBigText) { // only show latitude and longitute with big text size
                display.setTextSize(1);
                display.print("LT");
                display.setCursor(0, 20);
                display.print("LN");

                display.setTextSize(2);
                display.setCursor(13, 0);
                display.println(gps.location.lat(), 6);
                display.setCursor(13, 17);
                display.print(gps.location.lng(), 6);

                Serial.print("Latitude: ");
                Serial.println(gps.location.lat(), 6);
                Serial.print("Longitude: ");
                Serial.print(gps.location.lng(), 6);
            } else { // most robust view (time, num of sat, lat, long, speed)
                display.setTextSize(1);
                display.print("UTC ");
                display.print(gps.time.hour());
                display.print(":");
                display.print(gps.time.minute());
                display.print(" sat used: ");
                display.print(gps.satellites.value());

                display.setCursor(0, 8);
                display.print("Latitude:  ");
                display.println(gps.location.lat(), 6);
                display.setCursor(0, 16);
                display.print("Longitude: ");
                display.println(gps.location.lng(), 6);

                display.setCursor(0, 24);
                display.print("speed: ");
                display.print(gps.speed.kmph());
                display.print("km/h");
            }
        } else {
            display.setTextSize(1);
            display.println("Location not available");

            Serial.println("Location: Not Available");
        }
    } else {
        Serial.println("showing check distance");
        if(millis() - GPSInfoORDistanceCheckTimer >= 1000) {
            showGPSInfo = true;
            GPSInfoORDistanceCheckTimer = millis();
        }

        display.setTextSize(1);
        display.println("Currently check whether within:");
        display.setTextSize(3);
        display.setCursor(65, 10);
        display.print(radiusDistanceCheck);
        display.println("M");
    }

    display.display();
}

double toRadians(double degrees) {
    return degrees * M_PI / 180.0;
}

double calculateDistance(double latA, double lonA, double latB, double lonB) {
    // Convert coordinates from degrees to radians
    latA = toRadians(latA);
    lonA = toRadians(lonA);
    latB = toRadians(latB);
    lonB = toRadians(lonB);

    // Calculate differences between latitudes and longitudes
    double deltaLat = latB - latA;
    double deltaLon = lonB - lonA;

    // Apply Haversine formula
    double a = pow(sin(deltaLat / 2), 2) + cos(latA) * cos(latB) * pow(sin(deltaLon / 2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double distance = EARTH_RADIUS * c * 1000;

    return distance; // in meters
}

// determine direction of moving (start (first stop) to end (last stop), or end to start)
void determineDirection() {
    // get indexes of distances of stop last remembered
    uint8_t indexLastClosest, indexLastAdjacent;
    for (uint8_t i; i < coordinates.getStopsNum(); i++) {
        if (lastClosestStop == index_of_shortest[i]) {
            indexLastClosest = i;
        }
        if (adjacentToLastClosestStop == index_of_shortest[i]) {
            indexLastAdjacent = i;
        }
    }

    // if to close to a stop then skip direciton determination because there might be indeterminacy
    if(lastDistanceToClosestStop > 20 && lastDistanceAdjacentToClosestStop > 20) { 
        // lastClosest decreased, adjacent increased,
        // hence moving towards lastClosest
        if (lastDistanceToClosestStop - distances[indexLastClosest] >= 2 && distances[indexLastAdjacent] - lastDistanceAdjacentToClosestStop >= 2) {
            // if lastClosest is higher numbered stop then the bus's moving from start to end
            if (lastClosestStop > adjacentToLastClosestStop) {
                startToEnd = true;
            } else {  // moving towards lastClosest, which is lower number stop, means moving from end to start
                startToEnd = false;
            }
        }

        // lastClosest increased, adjacent decreased,
        // hence moving away from lastClosest
        if (distances[indexLastClosest] - lastDistanceToClosestStop >= 2 && lastDistanceAdjacentToClosestStop - distances[indexLastAdjacent] >= 2) {
            // if lastClosest is higher numbered stop and bus goes away from it then the bus's moving from end to start
            if (lastClosestStop > adjacentToLastClosestStop) {
                startToEnd = true;
            } else {  // moving away from lastClosest, which is lower number stop, means moving from end to start
                startToEnd = false;
            }
        }
    }

    // record info for the next run
    lastClosestStop = index_of_shortest[0];

    if(index_of_shortest[0] + 1 == index_of_shortest[1] || index_of_shortest[0] - 1 == index_of_shortest[1]) {
        adjacentToLastClosestStop = index_of_shortest[1];
        lastDistanceToClosestStop = distances[0];
        lastDistanceAdjacentToClosestStop = distances[1];
    } else if(index_of_shortest[0] + 1 == index_of_shortest[2] || index_of_shortest[0] - 1 == index_of_shortest[2]) {
        adjacentToLastClosestStop = index_of_shortest[2];
        lastDistanceToClosestStop = distances[0];
        lastDistanceAdjacentToClosestStop = distances[2];
    }
}

template <typename T>
void swap(T &a, T &b) {
    T temp = a;
    a = b;
    b = temp;
}
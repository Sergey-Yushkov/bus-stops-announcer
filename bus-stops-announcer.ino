#define BUTTON_PIN 9

#include <EncButton.h>
#include <SoftwareSerial.h>
#include <DFMiniMp3.h> // https://github.com/Makuna/DFMiniMp3
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>

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

#define RX_GPS 3 // arduino rx pin for gps (gps's tx)
#define TX_GPS 2 // arduino tx pin for gps (gps's rx)
#define GPSBaud 9600 // Default baud of NEO-6M is 9600
SoftwareSerial gpsSerial(RX_GPS, TX_GPS); // Create a software serial port called "gpsSerial"
TinyGPSPlus gps;
#define EARTH_RADIUS 6371  // Earth's radius in kilometers

Button btn(BUTTON_PIN);


class Mp3Notify; // forward declare the notify class, just the name

// typedef DFMiniMp3<HardwareSerial, Mp3Notify> DfMp3; // define a handy type using serial and our notify class
// DfMp3 dfmp3(Serial1); // instance a DfMp3 object

// Some arduino boards only have one hardware serial port, so a software serial port is needed instead.
// comment out the above definitions and use these
SoftwareSerial secondarySerial(5, 4); // RX (DF player's TX), TX (DF player's RX)
typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;
DfMp3 dfmp3(secondarySerial);

bool increaseRadiusDistanceCheck = true;
uint16_t radiusDistanceCheck = 5;
uint16_t volume;
uint16_t trackNumber = 1;
uint16_t trackCountInFolder1;
bool secondFinishCall = false;
bool gpsInfoBigText = false;
bool showDistancesInsteadOfCord = true;
double distances[6] = {0};
uint64_t stopPassedTime = 0;
uint64_t GPSInfoORDistanceCheckTimer = 0;
bool showGPSInfo = true;

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
            stopPassedTime = millis();
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
    btn.setBtnLevel(HIGH);
    btn.setStepTimeout(350);

    Serial.begin(57600);

    Serial.println("initializing DF player...");
    dfmp3.begin();
    // for boards that support hardware arbitrary pins
    // dfmp3.begin(6, 5); // RX, TX

    // during development, it's a good practice to put the module
    // into a known state by calling reset().
    // You may hear popping when starting and you can remove this
    // call to reset() once your project is finalized
    dfmp3.reset();

    uint16_t version = dfmp3.getSoftwareVersion();
    Serial.print("version ");
    Serial.println(version);

    // uint16_t volume = dfmp3.getVolume();
    volume = dfmp3.getVolume();
    Serial.print("volume was ");
    Serial.println(volume);
    Serial.println("now it's set to 15");
    dfmp3.setVolume(15);
    volume = 15;

    uint16_t count = dfmp3.getTotalTrackCount(DfMp3_PlaySource_Sd);
    Serial.print("files total ");
    Serial.println(count);

    trackCountInFolder1 = dfmp3.getFolderTrackCount(1);
    Serial.print("tracks in folder '01'");
    Serial.println(trackCountInFolder1);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
    }

    // Clear the buffer
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font

    // Start the software serial port at the GPS's default baud
    gpsSerial.begin(GPSBaud);
}

void loop() {
    btn.tick();
    dfmp3.loop();

    if(btn.hasClicks(1)) {
        if(showDistancesInsteadOfCord) {
            gpsInfoBigText = true;
            showDistancesInsteadOfCord = false;
        } else if(gpsInfoBigText) {
            gpsInfoBigText = false;
        } else {
            showDistancesInsteadOfCord = true;
        }
    }

    if(btn.step()) {
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

    if(btn.releaseStep()) {
        increaseRadiusDistanceCheck = !increaseRadiusDistanceCheck;
    }

    if(showGPSInfo) {
        while (gpsSerial.available() > 0) {
            if (gps.encode(gpsSerial.read())) {
                distances[0] = calculateDistance(gps.location.lat(), gps.location.lng(), 51.233362756292046, 33.203013111690495);
                distances[1] = calculateDistance(gps.location.lat(), gps.location.lng(), 51.23774695200957, 33.20839631107007);
                distances[2] = calculateDistance(gps.location.lat(), gps.location.lng(), 51.239598177386306, 33.20508454830953);
                distances[3] = calculateDistance(gps.location.lat(), gps.location.lng(), 51.24026715384555, 33.18343797286908);
                distances[4] = calculateDistance(gps.location.lat(), gps.location.lng(), 51.229328194391684, 33.18545848091145);
                distances[5] = calculateDistance(gps.location.lat(), gps.location.lng(), 51.22499374814668, 33.192970757259225);

                displayInfo();

                if(millis() - stopPassedTime >= 10000) {
                    if(distances[0] <= radiusDistanceCheck) { // лісового, світлофор навпроти коледжу
                        stopPassedTime = millis();
                        dfmp3.playFolderTrack16(2, 1);
                    } else if(distances[1] <= radiusDistanceCheck) { // перехрестя на проспекті миру навпроти коня
                        dfmp3.playFolderTrack16(2, 2);
                        stopPassedTime = millis();
                    } else if(distances[2] <= radiusDistanceCheck) { // круг біля краєзнавчого музею та екомаркета
                        dfmp3.playFolderTrack16(2, 3);
                        stopPassedTime = millis();
                    } else if(distances[3] <= radiusDistanceCheck) { // перехрестя успенсько-троїцька клубна
                        dfmp3.playFolderTrack16(2, 4);
                        stopPassedTime = millis();
                    } else if(distances[4] <= radiusDistanceCheck) { // нова пошта №3
                        dfmp3.playFolderTrack16(2, 5);
                        stopPassedTime = millis();
                    } else if(distances[5] <= radiusDistanceCheck) { // перехрестя шляхопровід (вул свободи пр. миру)
                        dfmp3.playFolderTrack16(2, 6);
                        stopPassedTime = millis();
                    }
                }
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
        if(millis() - GPSInfoORDistanceCheckTimer >= 3000) {
            showGPSInfo = false;
            GPSInfoORDistanceCheckTimer = millis();
        }

        if (gps.location.isValid()) {
            if(showDistancesInsteadOfCord) {
                display.setTextSize(1);
                for(byte i = 0; i < 2; i++) {
                    for (byte j = 0; j < 3; j++) {
                        byte cordNumber = j+(i*3);
                        display.setCursor(i * 64, j * 10);
                        display.print(cordNumber);
                        display.print(") ");
                        display.print(distances[cordNumber], 1);
                    }
                }
            } else if(gpsInfoBigText) {
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
            } else {
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
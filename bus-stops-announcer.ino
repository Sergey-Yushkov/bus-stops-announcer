#define BUTTON_PIN 9

#include <EncButton.h>
#include <SoftwareSerial.h>
#include <DFMiniMp3.h> // https://github.com/Makuna/DFMiniMp3
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

Button btn(BUTTON_PIN);


class Mp3Notify; // forward declare the notify class, just the name

// typedef DFMiniMp3<HardwareSerial, Mp3Notify> DfMp3; // define a handy type using serial and our notify class
// DfMp3 dfmp3(Serial1); // instance a DfMp3 object

// Some arduino boards only have one hardware serial port, so a software serial port is needed instead.
// comment out the above definitions and use these
SoftwareSerial secondarySerial(5, 4); // RX (DF player's TX), TX (DF player's RX)
typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;
DfMp3 dfmp3(secondarySerial);

bool increaseVolume = true;
uint16_t volume;
uint16_t trackNumber = 1;
uint16_t trackCountInFolder1;
bool secondFinishCall = false;

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
            if(secondFinishCall) {
                trackNumber += 1;
                if(trackNumber > trackCountInFolder1) trackNumber = 1;

                dfmp3.playFolderTrack16(1, trackNumber);

                secondFinishCall = false;
            } else {
                secondFinishCall = true;
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
    btn.setBtnLevel(HIGH);

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
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setCursor(0, 0);     // Start at top-left corner
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    display.write("Hello world!");
    display.setCursor(20, 20);
    display.write("TEST");
    display.display();

    // Start the software serial port at the GPS's default baud
    gpsSerial.begin(GPSBaud);
}

void loop() {
    btn.tick();
    dfmp3.loop();

    if(btn.hasClicks(1)) {
        if(dfmp3.getStatus().state == DfMp3_StatusState_Paused) {
            dfmp3.start();
            Serial.println("playing resumed");
        } else {
            dfmp3.pause();
            Serial.println("playing paused");
        }
    }

    if(btn.hasClicks(2)) {
        trackNumber++;
        if(trackNumber > trackCountInFolder1) trackNumber = 1;
        dfmp3.playFolderTrack16(1, trackNumber);
        Serial.println("next track");
    }

    if(btn.hasClicks(3)) {
        trackNumber--;
        if(trackNumber == 0) trackNumber = trackCountInFolder1;
        dfmp3.playFolderTrack16(1, trackNumber);
        Serial.println("prev track");
    }

    if(btn.step()) {
        if(increaseVolume) {
            dfmp3.increaseVolume();
            Serial.print("Volume increased, current is ");
            // Serial.println(dfmp3.getVolume());
            volume++;
            Serial.println(volume);
        } else {
            dfmp3.decreaseVolume();
            Serial.print("Volume decreased, current is ");
            // Serial.println(dfmp3.getVolume());
            volume--;
            Serial.println(volume);
        }
    }

    if(btn.releaseStep()) {
        increaseVolume = !increaseVolume;
    }

    while (gpsSerial.available() > 0)
        Serial.write(gpsSerial.read());
}
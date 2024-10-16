#ifndef COORDINATES_H
#define COORDINATES_H

#include <Arduino.h>

#define LINES 2
#define STOPS_MAX 25
#define LINE0_STOPS_N 25
#define LINE1_STOPS_N 19

uint8_t lines[LINES] = {8, 5};

const double line0[LINE0_STOPS_N][2] PROGMEM = { // coordinates from start to end for bus line 8 in konotop
    // 24 stops
    {51.21866, 33.14858}, 
    {51.21899, 33.15551}, 
    {51.21953, 33.16655},
    {51.21981, 33.17162},
    {51.22024, 33.1789},
    {51.220699, 33.187944},
    {51.221239, 33.192100},
    {51.22531, 33.19343},
    {51.22763, 33.1963}, 
    {51.23116, 33.20053},
    {51.23327, 33.20304},
    {51.23549, 33.20579},
    {51.23879, 33.20991},
    {51.24014, 33.21327},
    {51.24098, 33.21915},
    {51.24523, 33.21985},
    {51.247, 33.22023},
    {51.24849, 33.21674},
    {51.24888, 33.21406},
    {51.251185, 33.209998},
    {51.25324, 33.20563},
    {51.25522, 33.20128},
    {51.2562, 33.19881},
    {51.25985, 33.20091},
    {51.26221, 33.20336},
};

const double line0_endToStart[LINE0_STOPS_N][2] PROGMEM = {
    {51.21866, 33.14858}, 
    {51.21899, 33.15551}, 
    {51.21953, 33.16655},
    {51.21981, 33.17162},
    {51.22024, 33.1789},
    {51.22076, 33.18776},
    {51.22113, 33.19187},
    {51.22561, 33.19347},
    {51.22712, 33.19522},
    {51.22977, 33.19861},
    {51.23302, 33.20249},
    {51.23559, 33.20576},
    {51.23819, 33.20885},
    {51.24016, 33.21265},
    {51.24104, 33.21892},
    {51.2459, 33.21986},
    {51.24764, 33.22007},
    {51.24834, 33.2172},
    {51.24877, 33.21384},
    {51.2517, 33.20891},
    {51.253319, 33.205455},
    {51.25515, 33.2009},
    {51.25626, 33.19826},
    {51.25986, 33.20082},
    {51.26224, 33.20339},
};

const double line1[LINE1_STOPS_N][2] PROGMEM = { // coordinates from start to end for bus line 5 in konotop
    // 19 stops
    {51.24005957106069, 33.1752101278922}, 
    {51.23990021183767, 33.18335474996154}, 
    {51.23340713079708, 33.1841712344273},
    {51.22873889362563, 33.18554381709432},
    {51.22522232715611, 33.18663457930069},
    {51.22556099003479, 33.1935971041912},
    {51.22760703620993, 33.1961220084035},
    {51.23087011821699, 33.20005267900728}, 
    {51.23335386922896, 33.20309224655335},
    {51.23552500026175, 33.20572228760831},
    {51.23883375573477, 33.20988542040044},
    {51.24017441201393, 33.213292087431974},
    {51.23999313122165, 33.21876677960294},
    {51.23723159514404, 33.21838363511894},
    {51.23382425087031, 33.21544608313837},
    {51.23054375718303, 33.21342569633776},
    {51.22627836240819, 33.2168888148671},
    {51.22149393720388, 33.22012166107407},
    {51.21809750527479, 33.2283861397093},
};

const double line1_endToStart[LINE1_STOPS_N][2] PROGMEM = { 
    {51.24005957106069, 33.1752101278922}, 
    {51.23990021183767, 33.18335474996154}, 
    {51.23340713079708, 33.1841712344273},
    {51.22873889362563, 33.18554381709432},
    {51.22522232715611, 33.18663457930069},
    {51.22556099003479, 33.1935971041912},
    {51.22760703620993, 33.1961220084035},
    {51.23087011821699, 33.20005267900728}, 
    {51.23335386922896, 33.20309224655335},
    {51.23552500026175, 33.20572228760831},
    {51.23883375573477, 33.20988542040044},
    {51.24017441201393, 33.213292087431974},
    {51.23999313122165, 33.21876677960294},
    {51.23723159514404, 33.21838363511894},
    {51.23382425087031, 33.21544608313837},
    {51.23054375718303, 33.21342569633776},
    {51.22627836240819, 33.2168888148671},
    {51.22149393720388, 33.22012166107407},
    {51.21809750527479, 33.2283861397093},
};


class Coordinates {
    private:
        uint8_t lineIndex = 0;

        double getCoord(uint8_t stopNumber, uint8_t latORlong, bool startToEnd) {
            switch(lineIndex) {
                case 0:
                    if(startToEnd) {
                        return pgm_read_float(&line0[stopNumber][latORlong]);
                    } else {
                        return pgm_read_float(&line0_endToStart[stopNumber][latORlong]);
                    }
                case 1:
                    if(startToEnd) {
                        return pgm_read_float(&line1[stopNumber][latORlong]);
                    } else {
                        return pgm_read_float(&line1_endToStart[stopNumber][latORlong]);
                    }
                case 2:
                    break;
                case 3:
                    break;
            }
        }

    public:
        void setLineNumber(uint8_t number) {
            for(uint8_t i = 0; i < LINES; i++) {
                if(lines[i] == number) {
                    lineIndex = i;
                }
            }
        }

        void nextLine() {
            lineIndex == LINES-1 ? lineIndex = 0 : lineIndex++;
        }

        void prevLine() {
            lineIndex == 0 ? lineIndex = LINES-1 : lineIndex--;
        }

        uint8_t currentLine() {
            return lines[lineIndex];
        }

        double getLat(uint8_t stopNumber, bool startToEnd) {
            return getCoord(stopNumber, 0, startToEnd);
        }

        double getLng(uint8_t stopNumber, bool startToEnd) {
            return getCoord(stopNumber, 1, startToEnd);
        }

        uint8_t getStopsNum() {
            switch(lineIndex) {
                case 0:
                    return LINE0_STOPS_N;
                case 1:
                    return LINE1_STOPS_N;
            }
        }

};


#endif // COORDINATES_H
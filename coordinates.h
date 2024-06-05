#ifndef COORDINATES_H
#define COORDINATES_H

#include <Arduino.h>

#define LINES 2
#define STOPS_MAX 24
#define LINE0_STOPS_N 24
#define LINE1_STOPS_N 19

const double line0[LINE0_STOPS_N][2] PROGMEM = { // coordinates from start to end for bus line 8 in konotop
    // 24 stops
    {51.21901985314071, 33.156171872003256}, 
    {51.21864645597891, 33.14909845124785}, 
    {51.21958217277598, 33.16774847422319},
    {51.21989111434532, 33.172230827074614},
    {51.22020874754201, 33.1784640397636},
    {51.22069919522844, 33.18794469304724},
    {51.22123921119236, 33.19210074026167},
    {51.22556099003479, 33.1935971041912},
    {51.22760703620993, 33.1961220084035},
    {51.23087011821699, 33.20005267900728},
    {51.23335386922896, 33.20309224655335},
    {51.23552500026175, 33.20572228760831}, 
    {51.23883375573477, 33.20988542040044}, 
    {51.24017441201393, 33.213292087431974}, 
    {51.24083250271808, 33.21905104584709},
    {51.24540913658022, 33.21986197285737},
    {51.2470224033654, 33.22017415518687}, 
    {51.24855039379855, 33.216563443646095}, 
    {51.248860768445375, 33.214146488006776}, 
    {51.25093673989553, 33.21052400988583},
    {51.25477757301808, 33.202077474179426}, 
    {51.256194962619645, 33.198916905536635}, 
    {51.25944851734781, 33.20057374363919}, 
    {51.26227371130047, 33.20349468648514},
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


class Coordinates {
    private:
        uint8_t line = 0;

        double getCoord(uint8_t stopNumber, uint8_t latORlong) {
            switch(line) {
                case 0:
                    return pgm_read_float(&line0[stopNumber][latORlong]);
                case 1:
                    return pgm_read_float(&line1[stopNumber][latORlong]);
                case 2:
                    break;
                case 3:
                    break;
            }
        }

    public:
        void setLineNumber(uint8_t number) {
            line = number;
        }

        void nextLine() {
            line == LINES-1 ? line = 0 : line++;
        }

        void prevLine() {
            line == 0 ? line = LINES-1 : line--;
        }

        uint8_t currentLine() {
            return line;
        }

        double getLat(uint8_t stopNumber) {
            return getCoord(stopNumber, 0);
        }

        double getLng(uint8_t stopNumber) {
            return getCoord(stopNumber, 1);
        }

        uint8_t getStopsNum() {
            switch(line) {
                case 0:
                    return LINE0_STOPS_N;
                case 1:
                    return LINE1_STOPS_N;
            }
        }

};


#endif // COORDINATES_H
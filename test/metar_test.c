#include "metar.h"

void test_time_parser() {
    const char* test_cases[] = {
        "051200Z",  // Valid
        "311859Z",  // Valid (edge cases)
        "010000Z",  // Valid
        "051200",   // Missing Z
        "0512Z",    // Too short
        "0512000Z", // Too long
        "991200Z",  // Invalid day
        "052500Z",  // Invalid hour
        "051060Z",  // Invalid minute
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        MetarTime time;
        int result = parse_metar_time(test_cases[i], &time);

        metar_printf("Test '%s': %s\n", test_cases[i],
                    result ? "SUCCESS" : "FAIL");

        if (result) {
            metar_printf("  Day: %d, Time: %02d:%02d\n",
                        time.day, time.hour, time.minute);
        }
    }
}

void test_wind_extractor() {
    const char* test_cases[] = {
        "03003MPS",    // Valid: dir="030", speed="03"
        "12015MPS",    // Valid: dir="120", speed="15"
        "VRB05MPS",    // Valid: dir="VRB", speed="05"
        "ABV49MPS",
        "ABV99KT",
        "00000MPS",    // Valid: dir="000", speed="00"
        "03012G15MPS", // Valid: dir="030", speed="12"
        "270100MPS",   // Valid: dir="270", speed="100" (сильный ветер)
        "12015KT",     // Invalid: wrong units
        "12MPS",       // Invalid: too short
        "ABCDEFGH",    // Invalid: wrong format
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        MetarWind wind;
        int result = parse_metar_wind(test_cases[i], &wind);
        metar_printf(" Wind extraction for %s\n", test_cases[i]);
        if (result) {
            metar_printf(" Failed\n");
        } else {
            print_metar_wind(&wind);
        }
    }
}

void metar_full_test() {
    const char* metar_test_cases[] = {
        "UWGG 231700Z 23002MPS 9999 OVC012 05/04 Q1017 R36R/190060 NOSIG",
        "UUEE 231700Z 14004MPS 110V170 9999 SCT044 05/01 Q1014 R24L/CLRD62 R24C/CLRD62 NOSIG",
        "ULLI 231700Z 14004MPS 110V180 9999 -RA BKN026 BKN043 07/02 Q1006 R10R/090060 NOSIG",
        "EFKS 231720Z AUTO 18009KT 9999 OVC005 03/02 Q1003",
        "SBMG 231700Z 02010G20KT 320V070 CAVOK 28/09 Q1018",
        "LKPD 231700Z 25008KT 9999 -RA FEW015 OVC050 12/11 Q0994 RERA NOSIG RMK BLU BLU",
        "ZSPD 231700Z 36007MPS CAVOK 17/12 Q1026 NOSIG",
        NULL
    };

    for (int i = 0; metar_test_cases[i] != NULL; i++) {

        int result = process_metar(metar_test_cases[i]);

        metar_printf("Test '%s': %s\n", metar_test_cases[i],
                    result ? "SUCCESS" : "FAIL");
    }
}




int main(){
    test_wind_extractor();

    test_time_parser();

    metar_full_test();
    return 0;
}
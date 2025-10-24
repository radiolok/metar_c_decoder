#ifndef __METAR_C_DECODER_H__
#define __METAR_C_DECODER_H__

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

// Максимальное количество токенов в METAR
#define MAX_TOKENS 20
// Максимальная длина одного токена
#define MAX_TOKEN_LEN 16

// Максимальная длина для компонентов ветра
#define WIND_DIR_LEN 4    // 3 символа + \0
#define WIND_SPEED_LEN 4  // 3 символа + \0 (обычно 2-3 цифры)

typedef struct {
    char day[2];
    char hour[2];
    char minute[2];
} MetarTime;

// Коды погодных явлений (обновленные)
typedef enum {
    WEATHER_NONE = 0,   // Нет явлений
    WEATHER_FOG,        // Туман (1)
    WEATHER_HAZE,       // Дымка (2)
    WEATHER_HAIL,       // Град (3)
    WEATHER_RAIN,       // Дождь (4)
    WEATHER_SNOW,       // Снег (5)
    WEATHER_DRIFTING,   // Поземок (6)
    WEATHER_BLIZZARD,   // Метель (7)
    WEATHER_THUNDER     // Гроза (8) - теперь только для основного поля
} WeatherCode;

typedef struct {
    char speed[3];
    char gust_speed[3];
    char dir[4];
    bool mps;
    bool above;
    bool gust;
    bool vrb;
} MetarWind;

typedef struct {
    char icao[4] ;// ICAO airport code
    MetarTime time;
    bool automatic;
    MetarWind wind;
    char visibility[5];
    char wind_dir[4];       // Направление ветра
    char wind_speed[4];     // Скорость ветра
    char general[5];    // Общая видимость (4 символа + \0)
    char runway[5];     // Видимость на ВПП (4 символа + \0)
    WeatherCode weather;    // Основное явление (0-8)
    int is_thunder;        // Флаг грозы
    int is_icing;          // Флаг гололеда
    int is_complex;        // Флаг сложных условий
    // ... другие поля ...
} MetarData;

// Функция для кроссплатформенного вывода
void metar_printf(const char* format, ...);
int tokenize_metar(const char* raw, char tokens[][MAX_TOKEN_LEN], int max_tokens) ;
int process_metar(const char* raw_metar);
int process_tokens(char tokens[][MAX_TOKEN_LEN], int token_count, MetarData* out);
int parse_metar_time(const char* token, MetarTime* out_time) ;
int parse_metar_wind(const char* token, MetarWind* wind) ;

void print_metar_wind(const MetarWind* wind);

#endif
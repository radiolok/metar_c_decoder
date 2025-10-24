#include "metar.h"

/**
 * @brief Разбивает строку METAR на токены (вторая строка после перевода строки)
 *
 * @param raw Указатель на сырую строку METAR (с датой и данными)
 * @param tokens Массив для хранения результата (должен быть предварительно выделен)
 * @param max_tokens Максимальное количество токенов в массиве
 * @return int Количество найденных токенов, или -1 при ошибке
 */
int tokenize_metar(const char* raw, char tokens[][MAX_TOKEN_LEN], int max_tokens) {
    if (raw == NULL || tokens == NULL || max_tokens < 1) {
        return -1;
    }

    const char* start = strchr(raw, '\n');
    if (start == NULL) {
        start = raw;
    } else {
        start++;
    }

    int token_count = 0;
    const char* current = start;
    const char* end = start + strlen(start);

    while (current < end && token_count < max_tokens) {
        while (*current == ' ' && current < end) current++;
        if (current >= end) break;

        const char* token_end = current;
        while (*token_end != ' ' && *token_end != '\0' && *token_end != '\r' && *token_end != '\n' && token_end < end) {
            token_end++;
        }

        size_t token_len = token_end - current;
        if (token_len >= MAX_TOKEN_LEN) {
            token_len = MAX_TOKEN_LEN - 1;
        }

        strncpy(tokens[token_count], current, token_len);
        tokens[token_count][token_len] = '\0';
        token_count++;

        current = token_end;
    }
    return token_count;
}

int process_metar(const char* raw_metar) {
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int count = tokenize_metar(raw_metar, tokens, MAX_TOKENS);

    if (count > 0) {
        metar_printf("Successfully parsed METAR:\n");
        for (int i = 0; i < count; i++) {
            metar_printf("Token %d: %s\n", i, tokens[i]);
        }
    } else {
        metar_printf("METAR parsing failed\n");
        return -1;
    }

    MetarData data;
    process_tokens(tokens, count, &data);

    return 0;
}

void metar_printf(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);

    // Форматирование строки
    vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[sizeof(buffer) - 1] = '\0'; // Гарантируем завершение строки

    va_end(args);

    // Вывод в зависимости от платформы
    #ifdef METAR_PLATFORM_ESP32
    Serial.print(buffer);
    #else
    printf("%s", buffer);
    #endif
}

/**
 * @brief Парсит время из токена METAR
 *
 * @param token Строка с временем в формате DDHHMMZ
 * @param out_time Указатель на структуру для записи результата
 * @return 1 - успешный парсинг, 0 - ошибка формата
 */
int parse_metar_time(const char* token, MetarTime* out_time) {
    if (token == NULL || out_time == NULL)
        return -1;
    if (strlen(token) != 7)
        return -1;
    if (token[6] != 'Z') //Not UTC
        return -1;
    out_time->day[0] = token[0];
    out_time->day[1] = token[1];
    out_time->hour[0] = token[2];
    out_time->hour[1] = token[3];
    out_time->minute[0] = token[4];
    out_time->minute[1] = token[5];
    return 0;
}

/**
 * @brief Извлекает направление и скорость ветра из токена
 *
 * @param token Токен с данными о ветре (например, "03003MPS")
 * @param dir_buf Буфер для направления (минимум 4 байта)
 * @param speed_buf Буфер для скорости (минимум 4 байта)
 * @return 1 - успешное извлечение, 0 - ошибка формата
 */
int parse_metar_wind(const char* token, MetarWind* wind) {
    if (token == NULL || wind == NULL) {
        return -1;
    }
    size_t len = strlen(token);
    //Minimal length 3(dir) + 2(speed)+2(unit)
    if (len < 7) {
        return -1;
    }
    const char* curr = token;
    const char* max = token + len;
    //direction
    strncpy(wind->dir, curr, 3);
    wind->dir[3] = '\0';
    curr = curr + 3;
    if (strncmp(wind->dir, "VRB", 3) == 0) {
        wind->vrb = true;
        wind->dir[0] = '\0';
    } else {
        wind->vrb = false;
    }
    if (strncmp(wind->dir, "ABV", 3) == 0) {
        wind->above = true;
        wind->dir[0] = '\0';
    } else {
        wind->above = false;
    }
    //speed
    strncpy(wind->speed, curr, 2);
    wind->speed[2] = '\0';
    curr = curr + 2;

    if (strncmp(curr, "G", 1) == 0) {
        //gust enabled
        wind->gust = true;
        curr = curr + 1;
        strncpy(wind->gust_speed, curr, 2);
        wind->gust_speed[2] = '\0';
        curr = curr + 2;
    }
    else {
        wind->gust = false;
    }

    if (strncmp(curr, "MP", 2) == 0) {
        //gust enabled
        wind->mps = true;
    }
    else {
        wind->mps = false;
    }
    return 0;
}

void print_metar_wind(const MetarWind* wind) {
    metar_printf("Wind direction: %s\n", wind->dir);
    metar_printf("Wind speed: %s %s %s\n", (wind->above? "More than" : ""),wind->speed, (wind->mps? "mp/s" : "kt"));
    if (wind->vrb)
        metar_printf("Variable\n");
    if (wind->gust)
        metar_printf("Gust speed: %s %s\n", wind->gust_speed, (wind->mps? "mp/s" : "kt"));
}

/**
 * @brief Извлекает данные о видимости из токенов METAR
 *
 * @param tokens Массив токенов
 * @param token_count Количество токенов
 * @param start_index Индекс, с которого начинать поиск
 * @return VisibilityData Структура с данными о видимости
 */
void extract_visibility(char tokens[][MAX_TOKEN_LEN],
    char *general, char *runway, int token_count, int start_index) {

    // Флаг для пропуска следующих токенов после CAVOK
    int skip_next = 0;

    for (int i = start_index; i < token_count; i++) {
        const char* token = tokens[i];

        // Проверка на CAVOK (особый случай видимости)
        if (strcmp(token, "CAVOK") == 0) {
            strncpy(general, "CAV", 4);
            general[3] = '\0';
            skip_next = 1; // Следующие токены могут быть связаны с видимостью
            continue;
        }

        // Пропуск токенов после CAVOK
        if (skip_next) {
            // Если встретили новую группу данных, прерываем пропуск
            if (strlen(token) > 1 && isalpha(token[0])) {
                skip_next = 0;
            } else {
                continue;
            }
        }

        // Поиск видимости на ВПП (начинается с 'R')
        if (token[0] == 'R') {
            // Ищем разделитель '/'
            const char* slash = strchr(token, '/');
            if (slash != NULL) {
                // Копируем 4 символа после '/'
                const char* vis_start = slash + 1;
                size_t len = strlen(vis_start);
                if (len > 0) {
                    size_t copy_len = len > 4 ? 4 : len;
                    strncpy(runway, vis_start, copy_len);
                    runway[copy_len] = '\0';
                }
            }
        }
        // Общая видимость (числовое значение)
        else if (isdigit(token[0]) && strlen(token) <= 4) {
            if (strlen(general) == 0) { // Берем первое подходящее значение
                strncpy(general, token, 4);
                general[4] = '\0';
            }
        }
    }
}

void print_visibility(const char* general, const char* runway) {
    metar_printf("General visibility: %s\n", general);
    metar_printf("Runway visibility: %s\n", runway);
}

/**
 * @brief Извлекает погодные явления с учетом отдельных флагов
 *
 * @param tokens Массив токенов
 * @param token_count Количество токенов
 * @param start_index Стартовый индекс
 * @param out_thunder Флаг грозы (выходной параметр)
 * @param out_icing Флаг гололеда (выходной параметр)
 * @return WeatherCode Код основного явления
 */
WeatherCode extract_weather_flags(
    char tokens[][MAX_TOKEN_LEN],
    int token_count,
    int start_index,
    int* out_thunder,
    int* out_icing
) {
    *out_thunder = 0;
    *out_icing = 0;
    WeatherCode code = WEATHER_NONE;
    int complex_weather = 0; // Флаг сложных условий

    for (int i = start_index; i < token_count; i++) {
        const char* token = tokens[i];

        // Проверка на грозу (отдельный флаг)
        if (strstr(token, "TS") != NULL) {
            *out_thunder = 1;
            code = WEATHER_THUNDER; // Устанавливаем как основное явление
        }

        // Проверка на гололед (отдельный флаг)
        if (strstr(token, "FZ") != NULL) {
            *out_icing = 1;
        }

        // Определение основного явления (если еще не установлена гроза)
        if (code < WEATHER_THUNDER) {
            if (strstr(token, "BLSN") != NULL) {
                code = WEATHER_BLIZZARD;
            }
            else if (strstr(token, "DRSN") != NULL) {
                code = WEATHER_DRIFTING;
            }
            else if (strstr(token, "SN") != NULL) {
                if (code < WEATHER_SNOW) code = WEATHER_SNOW;
            }
            else if (strstr(token, "RA") != NULL) {
                if (code < WEATHER_RAIN) code = WEATHER_RAIN;
            }
            else if (strstr(token, "GR") != NULL || strstr(token, "GS") != NULL) {
                if (code < WEATHER_HAIL) code = WEATHER_HAIL;
            }
            else if (strstr(token, "BR") != NULL) {
                if (code < WEATHER_HAZE) code = WEATHER_HAZE;
            }
            else if (strstr(token, "FG") != NULL) {
                if (code < WEATHER_FOG) code = WEATHER_FOG;
            }
        }

        // Проверка на сложные условия (несколько явлений)
        if (strchr(token, '+') || strchr(token, '-') ||
            strstr(token, "VC") || strstr(token, "RE")) {
            complex_weather = 1;
        }

        // Прерывание при достижении температуры
        if (strchr(token, '/') != NULL) break;
    }

    // Обработка сложных условий
    if (complex_weather) {
        if (code == WEATHER_RAIN) return WEATHER_RAIN;
        if (code == WEATHER_SNOW) return WEATHER_SNOW;
    }

    return code;
}

int process_tokens(char tokens[][MAX_TOKEN_LEN], int token_count, MetarData* out) {
    // Индекс первого токена с данными (после кода станции)
    // tokens[0] - код станции ("UWGG")
    // tokens[1] - время ("051200Z")

    if (token_count < 5) return 0; // Минимальное количество токенов

    // Копируем код станции
    strncpy(out->icao, tokens[0], sizeof(out->icao) - 1);
    out->icao[sizeof(out->icao) - 1] = '\0';

    // Парсим время
    if (!parse_metar_time(tokens[1], &out->time)) {
        return -1;
    }

    if (!parse_metar_wind(tokens[2], &out->wind)) {
        return -1;
    }



    extract_visibility(tokens, out->general, out->runway, token_count, 2);

    // ... парсинг остальных полей ...
    print_visibility(out->general, out->runway);

    // Извлекаем погодные явления с флагами
    int thunder, icing;
    out->weather = extract_weather_flags(tokens, token_count, 3, &thunder, &icing);
    out->is_thunder = thunder;
    out->is_icing = icing;

    return 1;
}
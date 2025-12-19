#ifndef COLORS_H
#define COLORS_H

// ANSI Color Codes для терминала

// Основные цвета
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"

// Серверные сообщения (темно-серый/синий)
#define COLOR_SERVER  "\033[38;5;117m"      // Светло-голубой
#define COLOR_INFO    "\033[38;5;33m"       // Синий
#define COLOR_ERROR   "\033[38;5;196m"      // Красный
#define COLOR_SUCCESS "\033[38;5;34m"       // Зелёный

// Системные сообщения
#define COLOR_SYSTEM  "\033[38;5;245m"      // Светло-серый
#define COLOR_TIMESTAMP "\033[38;5;245m"    // Светло-серый для timestamps

// Цвета для пользователей (яркие и контрастные)
#define COLOR_USER_1  "\033[38;5;81m"       // Голубой
#define COLOR_USER_2  "\033[38;5;213m"      // Розовый
#define COLOR_USER_3  "\033[38;5;228m"      // Жёлтый
#define COLOR_USER_4  "\033[38;5;120m"      // Салатовый
#define COLOR_USER_5  "\033[38;5;219m"      // Лавандовый
#define COLOR_USER_6  "\033[38;5;216m"      // Персиковый
#define COLOR_USER_7  "\033[38;5;159m"      // Мятный
#define COLOR_USER_8  "\033[38;5;210m"      // Оранжевый
#define COLOR_USER_9  "\033[38;5;141m"      // Фиолетовый
#define COLOR_USER_10 "\033[38;5;86m"       // Бирюзовый

// Массив цветов пользователей для удобного доступа
extern const char* USER_COLORS[10];

#define USER_COLORS_COUNT 10

// Цвет для личных сообщений
#define COLOR_PM "\033[38;5;206m"           // Ярко-розовый

// Цвет для действий (join/leave/disconnect)
#define COLOR_ACTION "\033[38;5;178m"       // Золотистый

#endif // COLORS_H
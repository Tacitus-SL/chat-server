#ifndef COLORS_H
#define COLORS_H

/**
 * @file colors.h
 * @brief ANSI Color Codes for terminal output formatting.
 *
 * This file contains macros for coloring text in the client console.
 * It includes colors for server messages, users, errors, and system notifications.
 */

/* --- Basic Colors --- */
#define COLOR_RESET     "\033[0m"       /**< Reset all color attributes */
#define COLOR_BOLD      "\033[1m"       /**< Bold text */
#define COLOR_DIM       "\033[2m"       /**< Dim/Faint text */

/* --- Server Messages (Greys/Blues/Reds) --- */
#define COLOR_SERVER    "\033[38;5;117m" /**< Light Blue - General server info */
#define COLOR_INFO      "\033[38;5;33m"  /**< Blue - Informational messages */
#define COLOR_ERROR     "\033[38;5;196m" /**< Red - Errors and warnings */
#define COLOR_SUCCESS   "\033[38;5;34m"  /**< Green - Success messages */

/* --- System Messages --- */
#define COLOR_SYSTEM    "\033[38;5;245m" /**< Light Grey - System internal msgs */
#define COLOR_TIMESTAMP "\033[38;5;245m" /**< Light Grey - Message timestamps */

/* --- User Colors (Bright and High Contrast) --- */
#define COLOR_USER_1    "\033[38;5;81m"  /**< Cyan */
#define COLOR_USER_2    "\033[38;5;213m" /**< Pink */
#define COLOR_USER_3    "\033[38;5;228m" /**< Light Yellow */
#define COLOR_USER_4    "\033[38;5;120m" /**< Lime Green */
#define COLOR_USER_5    "\033[38;5;219m" /**< Lavender */
#define COLOR_USER_6    "\033[38;5;216m" /**< Peach */
#define COLOR_USER_7    "\033[38;5;159m" /**< Mint */
#define COLOR_USER_8    "\033[38;5;210m" /**< Orange */
#define COLOR_USER_9    "\033[38;5;141m" /**< Purple */
#define COLOR_USER_10   "\033[38;5;86m"  /**< Turquoise */

/**
 * @brief Array of user colors for deterministic color assignment.
 */
extern const char *USER_COLORS[10];

#define USER_COLORS_COUNT 10

/* --- Specific Context Colors --- */
#define COLOR_PM        "\033[38;5;206m" /**< Hot Pink - Private messages */
#define COLOR_ACTION    "\033[38;5;178m" /**< Gold - Join/Leave/Disconnect events */

#endif /* COLORS_H */
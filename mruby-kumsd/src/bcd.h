#pragma once

/* Positions of the values in a BCD encoded time. */
#define BCD_HOUR   0
#define BCD_MINUTE 1
#define BCD_SECOND 2
#define BCD_DAY    3
#define BCD_MONTH  4
#define BCD_YEAR   5

/* Converts a two digit BCD char into an integer. */
#define bcd_int(c) (((c) & 15) + (((c) >> 4) & 15) * 10)

/* Converts an integer into a two digit BCD char. */
#define int_bcd(i) ((((i) / 10) << 4) | ((i) % 10))

/* Checks, if a char is a valid two digit BCD char. */
#define is_bcd(c) (((c) & 15) < 10 && (((c) >> 4) & 15) < 10)

/* Checks if the BCD encoded time is a valid time. Returns 1 if the supplied
 * time is valid, 0 otherwise. */
extern int bcd_valid(const char *bcd);

#include "bcd.h"

#define leapyear(year) ((year) % 400 == 0 || ((year) % 4 == 0 && (year) % 100 != 0))
#define longmonth(month) ((((month) >> 3) ^ (month)) & 1)
int bcd_valid(const char *bcd)
{
  int a, b, c;
  if (!(is_bcd(bcd[BCD_HOUR]) && is_bcd(bcd[BCD_MINUTE]) && is_bcd(bcd[BCD_SECOND]) &&
    is_bcd(bcd[BCD_DAY]) && is_bcd(bcd[BCD_MONTH]) && is_bcd(bcd[BCD_YEAR])))
    return 0;
  a = bcd_int(bcd[BCD_HOUR]);
  b = bcd_int(bcd[BCD_MINUTE]);
  c = bcd_int(bcd[BCD_SECOND]);
  if (a > 23 || b > 59 || c > 60) return 0;
  a = bcd_int(bcd[BCD_DAY]);
  b = bcd_int(bcd[BCD_MONTH]);
  c = bcd_int(bcd[BCD_YEAR]);
  if (b < 1 || b > 12) return 0;
  if (!(1 <= a && (a <= 28 || (leapyear(c) && a <= 29) || (b != 2 && a <= 30) ||
    (longmonth(b) && a <= 31))))
    return 0;
  return 1;
}

#include "simple_time.h"

void timestamp_to_tm(uint64_t ts, struct tm* out)
{
    int64_t days_since_epoch = (int64_t)(ts / 86400);
    uint32_t secs_of_day = (uint32_t)(ts % 86400);

    /* days_since_epoch → y/m/d (Howard Hinnant civil_from_days) */
    int64_t  z = days_since_epoch + 719468;
    int64_t  era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int64_t  y = (int64_t)yoe + era * 400;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp = (5 * doy + 2) / 153;
    uint32_t d = doy - (153 * mp + 2) / 5 + 1;
    uint32_t m = mp + (mp < 10 ? 3 : -9);
    int64_t  year = y + (m <= 2 ? 1 : 0);

    /* 填充 struct tm */

    out->tm_sec = (int)(secs_of_day % 60);
    out->tm_min = (int)((secs_of_day / 60) % 60);
    out->tm_hour = (int)(secs_of_day / 3600);
    out->tm_mday = (int)d;
    out->tm_mon = (int)(m - 1);        /* 0-11 */
    out->tm_year = (int)(year - 1900);  /* since 1900 */
    out->tm_wday = (int)(((days_since_epoch + 4) % 7 + 7) % 7);

    static const int days_before_month[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    int leap = (m > 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0));
    out->tm_yday = days_before_month[m - 1] + (int)d - 1 + leap;
    out->tm_isdst = 0;  /* UTC */

}

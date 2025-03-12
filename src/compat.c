
#if __linux__
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#elif _WIN32
#include "compat.h"
#endif

int vapp_usleep(unsigned usec)
{
#if __linux__
    return usleep(usec);
#elif _WIN32
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * (__int64)usec);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
    return 0;
#else
    return -1;
#endif
}



int64_t time_usec(void)
{
#if __linux__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#elif _WIN32
    FILETIME ft;
    int64_t t;
    GetSystemTimeAsFileTime(&ft);
    t = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
    return t / 10 - 11644473600000000; /* Jan 1, 1601 */
#else
    return -1;
#endif
}

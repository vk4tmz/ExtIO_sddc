#ifndef PORTSLEEP_H
#define PORTSLEEP_H

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif


inline void sleep_ms(int milliseconds){ // cross-platform sleep function
#ifdef WIN32
    // fprintf(stderr,"WIN32: Sleep %d\n", milliseconds);
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    // fprintf(stderr,"POSIX: NanoSleep %d\n", milliseconds);
    //struct timespec trm, ts = {1 , 500};
    struct timespec trm, ts;
    if(milliseconds > 999)
    {
      ts.tv_sec = (int) (milliseconds / 1000);
      ts.tv_nsec = (milliseconds % 1000) * 1000000;
    } else {
      ts.tv_sec = (int) 0;
      ts.tv_nsec = (milliseconds % 1000) * 1000000;
    } 

    //nanosleep(&ts, NULL);
    nanosleep(&ts, &trm);

#else
    // fprintf(stderr,"SLEEP/uSLEEP: NanoSleep %d\n", milliseconds);
    if (milliseconds >= 1000)
      sleep(milliseconds / 1000);
    usleep((milliseconds % 1000) * 1000);
#endif
}

#endif
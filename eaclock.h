#ifndef EATIME_H
#define EATIME_H
#include <libavutil/mathematics.h>

class EaClock
{
public:
    EaClock();
    EaClock(int*queueSerial);
    ~EaClock();

    double getClock(EaClock *c);
    void setClockAt(EaClock*c,double pts,int serial,double time);
    void setClock(EaClock*c,double pts,int serial);
    void setClockSpeed(EaClock*c,double speed);
    void syncClockSlave(EaClock*c,EaClock*salve);

private:
    // clock base
    double pts;

    // clock base minus time at which we update the clock
    double ptsDrift;
    double lastUpdated;
    double speed;

    //clock is based on a packet with this serial
    int serial;
    int paused;

    //pointer to the current packet queue serial,used for obsolete clock detection
    int *queueSerial;


};

#endif // EATIME_H

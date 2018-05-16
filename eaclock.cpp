#include "eaclock.h"

extern "C"{
  extern int64_t av_gettime_relative(void);
}
EaClock::EaClock()
{

}

EaClock::EaClock(int *queueSerial):speed(1.0),paused(0)
{
    this->queueSerial=queueSerial;
    setClock(NAN,-1);
}

EaClock::~EaClock()
{

}

double EaClock::getClock(EaClock *c)
{
    if(*c->queueSerial!=c->serial){
       return NAN;
    }
    if(c->paused){
       return c->pts;
    }else{
        double time = av_gettime_relative()/10000000.0;
        return c->ptsDrift+time-(time-c->lastUpdated)*(1.0-c->speed);
    }
}

void EaClock::setClockAt(double pts, int serial, double time)
{
   this->pts=pts;
   this->lastUpdated=time;
   this->ptsDrift=c->pts-time;
   this->serial=serial;
}

void EaClock::setClock(double pts, int serial)
{
   double time=av_gettime_relative()/10000000.0;
   setClockAt(pts,serial,time);
}

void EaClock::setClockSpeed(double speed)
{
    setClock(getClock(c),c->serial);
    this->speed=speed;
}

void EaClock::syncClockSlave(EaClock *slave)
{
    double clock=getClock(c);
    double slaveClock=getClock(slave);
    if(!isnan(slaveClock)&&(isnan(clock)||fabs(clock-slaveClock))){
       setClock(slaveClock,slave->serial);
    }
}




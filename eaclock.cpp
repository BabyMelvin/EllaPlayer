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
    setClock(this,NAN,-1);
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

void EaClock::setClockAt(EaClock *c, double pts, int serial, double time)
{
   c->pts=pts;
   c->lastUpdated=time;
   c->ptsDrift=c->pts-time;
   c->serial=serial;
}

void EaClock::setClock(EaClock *c, double pts, int serial)
{
   double time=av_gettime_relative()/10000000.0;
   setClockAt(c,pts,serial,time);
}

void EaClock::setClockSpeed(EaClock *c, double speed)
{
    setClock(c,getClock(c),c->serial);
    c->speed=speed;
}

void EaClock::syncClockSlave(EaClock *c, EaClock *slave)
{
    double clock=getClock(c);
    double slaveClock=getClock(slave);
    if(!isnan(slaveClock)&&(isnan(clock)||fabs(clock-slaveClock))){
       setClock(c,slaveClock,slave->serial);
    }
}




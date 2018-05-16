#ifndef EAPACKETQUEUE_H
#define EAPACKETQUEUE_H
#include "eaavpackagelist.h"
#include <QMutex>
#include <QWaitCondition>
#include "eaavpacket.h"

#define int64_t __int64
class EaPacketQueue
{
    static AVPacket flushPkt;
public:
    EaPacketQueue();
    ~EaPacketQueue();

    int putPrivate(AVPacket*pkt);
    int put(AVPacket*pkt);
    int putNullPacket(int streamIndex);
    void flush();
    void abort();
    void start();
    int get(AVPacket*pkt,int block);


private:
    EaAVPackageList * firstPkt,*lastPkt;
    int NBPackets;
    int size;
    int64_t duration;
    int abortRequest;
    int serial;
    QMutex* mutex;
    QWaitCondition*cond;
};

static AVPacket EaPacketQueue::flushPkt=dynamic_cast<AVPacket*>(new EaAVPacket);

#endif // EAPACKETQUEUE_H

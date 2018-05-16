#ifndef EADECODER_H
#define EADECODER_H
#include "libavcodec/avcodec.h"
#include "eapacketqueue.h"
#include <QWaitCondition>
#include <QThread>
#include "eaframequeue.h"

#define int64_t __int64
class EaDecoder
{
public:
    EaDecoder();
    ~EaDecoder();

    void abort(EaFrameQueue*fq);
    void decodeFrame(AVFrame*frame,AVSubtitle*sub);
    void start(int(*fn)(void*),void*arg);
private:
    AVPacket pkt;
    EaPacketQueue *queue;
    AVCodecContext * avCtx;
    int pktSerial;
    int finished;
    int packetPending;
    QWaitCondition *emptyQueueCond;

    int64_t nextPts;
    AVRational nextPtsTB;

    int64_t startPts;
    AVRational startPtsTB;//time base
    AVRational nextPtTB;
    QThread *decoderTid;
};

#endif // EADECODER_H

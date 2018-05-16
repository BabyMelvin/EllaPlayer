#ifndef EAAVPACKET_H
#define EAAVPACKET_H
#include <libavcodec/avcodec.h>

class EaAVPacket:AVPacket
{
public:
    EaAVPacket();
    ~EaAVPacket();
};

#endif // EAAVPACKET_H

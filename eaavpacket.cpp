#include "eaavpacket.h"
#include "libavcodec/avcodec.h"

extern "C"{
    void av_init_packet(AVPacket *pkt);
    void av_packet_free(AVPacket **pkt);
}

EaAVPacket::EaAVPacket()
{
  av_init_packet(this);
  this->data=(__UINT8_C*)this;
}

EaAVPacket::~EaAVPacket()
{
    av_packet_free(&this);
}


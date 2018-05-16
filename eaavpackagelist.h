#ifndef EAAVPACKAGELIST_H
#define EAAVPACKAGELIST_H

#include <libavcodec/avcodec.h>

class EaAVPackageList
{
public:
    EaAVPackageList();
    ~EaAVPackageList();

    AVPacket pkt;
    EaAVPackageList* next;
    int serial;
};

#endif // EAAVPACKAGELIST_H

#ifndef EAFRAME_H
#define EAFRAME_H

extern "C"{
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavutil/rational.h>
}
#define __int64 int64_t

class EaFrame
{
public:
    EaFrame();
    ~EaFrame();

    AVFrame *frame;
    AVSubtitle sub;
    int serial;

    //presentation timestamp for the frame
    double pts;

    //estimate duration of the frame
    double duration;

    //byte postion of the frame in the input file
    int64_t pos;
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flipV;
};

#endif // EAFRAME_H

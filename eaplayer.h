#ifndef EAPLAYER_H
#define EAPLAYER_H
#include <QThread>
#include "libavformat/avformat.h"
#include "libavcodec/avfft.h"
#include "libavfilter/avfilter.h"
#include "eaclock.h"
#include "eaframequeue.h"
#include "eadecoder.h"
#include "eapacketqueue.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "eaaudioparams.h"
#include <QWaitCondition>
#include <QDebug>

#define int64_t __int64
#define int16_t __int16
#define uint16_t __UINT16_C
#define int8_t  __int8
#define uint8_t __UINT8_C
#define SAMPLE_ARRAY_SIZE (8 * 65536)
enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

class EaPlayer:public QObject
{
    Q_OBJECT
    static int      startupVolume;
    static int      av_sync_type = AV_SYNC_AUDIO_MASTER;
    static int64_t  start_time   = AV_NOPTS_VALUE;
    static int64_t  duration     = AV_NOPTS_VALUE;
public:
    EaPlayer();
    EaPlayer(char*fileName,AVInputFormat*inputFormat);
    ~EaPlayer();
private slots:
   void readThreadWork(const QString& parameter);
signals:
   void resultReadThread(const QString&result);
private:
    void streamComponentClose(int streamIndex);
    QThread* readTid;
    AVInputFormat* iformat;
    int abortRequest;
    int forceRefresh;
    int paused;
    int lastPaused;
    int queueAttachmentsReq;
    int seekReq;
    int seekFlags;
    int64_t seekPos;
    int64_t seekRel;
    int readPauseReturn;
    AVFormatContext *ic;
    int realtime;

    EaClock audClk;
    EaClock vidClk;
    EaClock extClk;

    EaFrameQueue pictQ;
    EaFrameQueue subpQ;
    EaFrameQueue sampQ;

    EaDecoder audDec;
    EaDecoder vidDec;
    EaDecoder subDec;

    int audioStream;
    int avSyncType;
    double audioClock;
    int audioClockSerail;

    //use for AV differentce average computation
    double audioDiffCum;

    double audioDiffAvgCoef;
    double audioDiffThrshold;
    int audioDiffAvgCount;
    AVStream* audioSt;
    EaPacketQueue audioQ;
    int audioHwBufSize;
    uint8_t* auidoBuf;
    uint8_t* audioBuf1;

    //in bytes
    unsigned int audioBufSize;
    unsigned int audioBuf1Size;
    int audioBufIndex;
    int audioWriteBufSize;
    int audioVolume;
    int muted;
    EaAudioParams audioSrc;
#if CONFIG_AVFILTER
    EaAudioParams audioFilterSrc;
#endif
    EaAudioParams audioTgt;
    SwrContext* swsCtx;
    int frameDropsEarly;
    int frameDropsLate;
    enum ShowMode{
       SHOW_MODE_NONE=-1,
        SHOW_MODE_VIDOE=0,
        SHOW_MODE_WAVES,
        SHOW_MODE_RDFT,
        SHOW_MODE_NB
    }showMode;

    int16_t sampleArray[SAMAPLE_ARRAY_SIZE];
    int lastIStart;
    RDFTContext* rdft;
    int rdftBits;
    FFTSample* rdftData;
    int xPos;
    double lastVisTime;
    //设置显示属性 todo

    int subtitleStream;
    AVStream* subtitleSt;
    EaPacketQueue subtitleQ;

    double frameTimer;
    double frameLastReturnedTime;
    double frameLastFilterDelay;
    int videoStream;
    AVStream* videoSt;
    EaPacketQueue videoQ;

    //maxmimu duration of a frame - above this we consider the jump a timestamp discontinuity
    double maxFrameDuration;
    SwsContext* imgConvertCtx;
    SwsContext* subConvetCtx;
    int eof;

    char* fileName;
    int width,height,xLeft,yTop;
    int step;
#if CONFIG_AVFILTER
    int vFilterIdx;
    AVFilterContext* inVideoFilter;
    AVFilterContext* outVideoFilter;
    AVFilterContext* inAudioFilter;
    AVFilterContext* outAudioFilter;
    AVFilterGraph*   aGraph;
#endif
    int lastVideoStream,lastAudioStream,lastSubtitleStream;
    QWaitCondition* continueReadThread;
};

static int EaPlayer::startupVolume=100;
static int EaPlayer::av_sync_type = AV_SYNC_AUDIO_MASTER;
static int64_t EaPlayer::start_time = AV_NOPTS_VALUE;
static int64_t EaPlayer::duration = AV_NOPTS_VALUE;
#endif // EAPLAYER_H

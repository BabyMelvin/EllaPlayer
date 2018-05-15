#ifndef EAFRAMEQUEUE_H
#define EAFRAMEQUEUE_H
#include <eaframe.h>
#include <QMutex>
#include <QWaitCondition>
#include <eapacketqueue.h>

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))
class EaFrameQueue
{
public:
    EaFrameQueue();
    EaFrameQueue(EaPacketQueue *pktq, int maxSize, int keepLast);
    ~EaFrameQueue();

    void frameQueueUnrefItem(EaFrame*vp);
    int frameQueueSignal();
    EaFrame* frameQueuePeek();
    EaFrame* frameQueuePeekNext();
    EaFrame* frameQueuePeekLast();
    EaFrame* frameQueuePeekWritable();
    EaFrame* frameQueuePeekReadable();
    void  frameQueuePush();
    void  frameQueueNext(EaFrameQueue*f);
    int   frameQueueNBRemaining(EaFrameQueue* f);
    int64_t frameQueueLastPos(EaFrameQueue*f);

private:
    EaFrame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int maxSize;
    int keepLast;
    int rindexShown;
    QMutex *mutex;
    QWaitCondition *cond;
    EaPacketQueue *pktq;
};

#endif // EAFRAMEQUEUE_H

#include "eaframequeue.h"
#include <eapacketqueue.h>
#include <libavutil/error.h>

extern "C"{
    extern void av_log(void* avcl, int level, const char *fmt, ...);
    extern void av_frame_free(AVFrame **frame);
}
EaFrameQueue::EaFrameQueue(){

}

EaFrameQueue::EaFrameQueue(EaPacketQueue* pktq,int maxSize,int keepLast)
{
    int i;
    this->mutex=new QMutex;
    if(!this->mutex){
       av_log(NULL,AV_LOG_FATAL,"qt create mutex");
       return AVERROR(ENOMEM);
    }
    if(!this->cond=new QWaitCondition){
       av_log(NULL,AV_LOG_FATAL,"qt create wait cond");
       return AVERROR(ENOMEM);
    }
    this->pktq=pktq;
    this->maxSize=FFMIN(maxSize,FRAME_QUEUE_SIZE);
    this->keepLast=!!keepLast;
    for(i=0;i<this->maxSize;i++){
       if(!(this->queue[i]=av_frame_alloc())){
          return AVERROR(ENOMEM);
       }
    }
}

EaFrameQueue::~EaFrameQueue()
{
    int i;
    for(i=0;i<this->maxSize;i++){
       EaFrame *vp=&this->queue[i];
       frameQueueUnrefItem(vp);
       av_frame_free(&vp->frame);
    }
    delete this->mutex;
    delete this->cond;
}

void EaFrameQueue::frameQueueUnrefItem(EaFrame *vp)
{
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

int EaFrameQueue::frameQueueSignal()
{
    this->mutex->lock();
    this->cond->wakeOne();
    this->mutex->unlock();
}

EaFrame *EaFrameQueue::frameQueuePeek()
{
    return &this->queue[(this->rindex+this->rindexShown)%this->maxSize];
}

EaFrame *EaFrameQueue::frameQueuePeekNext()
{
    return &this->queue[(this->rindex+this->rindexShown+1)%this->maxSize] ;
}

EaFrame *EaFrameQueue::frameQueuePeekLast()
{
    return &this->queue[this->rindex];
}

EaFrame *EaFrameQueue::frameQueuePeekWritable()
{
    //wait until we have space to put a new frame
    this->mutex->lock();
    while(this->size>=this->maxSize&&!this->pktq->abortRequest){
       this->cond->wait(this->mutex);
    }
    this->mutex->unlock();
    if(this->pkt->abortRequest){
       return NULL;
    }
    return &this->queue[this->windex];
}

EaFrame *EaFrameQueue::frameQueuePeekReadable()
{
    //wait until we have a readale a new frame
    this->mutex->lock();
    while(this->size-this->rindexShown<=0&&!this->pktq->abortRequest){
        this->cond->wait(this->mutex);
    }
    this->mutex->unlock();
    if(this->pktq->abortRequest){
       return NULL;
    }
    return &this->queue[(this->rindex+this->rindexShown)%this->maxSize];
}

void EaFrameQueue::frameQueuePush()
{
   if(++this->windex==this->maxSize){
      this->windex=0;
   }

   this->mutex->lock();
   this->size++;
   this->cond->wakeOne();
   this->mutex->unlock();
}

int EaFrameQueue::frameQueueNBRemaining()
{
    return this->size-this->rindexShown;
}

int64_t EaFrameQueue::frameQueueLastPos(EaFrameQueue* f)
{
   EaFrame* fp=&f->queue[f->rindex] ;
   if(f->rindexShow&&fp->serial==f->pktq->serial){
      return fp->pos;
   }else{
       return -1;
   }
}

void EaFrameQueue::frameQueueNext(EaFrameQueue* f)
{
   if(this->keepLast&&!this->rindexShown){
      this->rindexShown=1;
       return;
   }
   // todo
   frameQueueUnrefItem(this->queue[f->rindex]);
   if(++f->rindex==f->maxSize){
      f->rindex=0;
   }
   f->mutex->lock();
   f->size--;
   f->cond->wakeOne();
   f->mutex->unlock();
}






#include "eapacketqueue.h"
#include "libavutil/mem.h"

extern "C"{
    extern void av_packet_unref(AVPacket *pkt);
}

AVPacket EaPacketQueue::getFlushPkt()
{
    return flushPkt;
}

void EaPacketQueue::setFlushPkt(const AVPacket &value)
{
    flushPkt = value;
}
EaPacketQueue::EaPacketQueue()
{
    memset(q,0,sizeof(this));
    this->mutex=new QMutex;
    if(!this->mutex){
        av_log(NULL,AV_LOG_FATAL,"create mutex");
        return AVERROR(ENOMEM);
    }
    this->cond=new QWaitCondition;
    if(!this->cond){
        av_log(NULL,AV_LOG_FATAL,"create cond");
        return AVERROR(ENOMEM);
    }
    this->abortRequest=1;
    return 0;
}

EaPacketQueue::~EaPacketQueue()
{
    flush();
    delete mutex;
    delete cond;
}

int EaPacketQueue::putPrivate(AVPacket *pkt)
{
    EaAVPackageList *pkt1;
   if(this->abortRequest){
      return -1;
   }
   pkt1=av_malloc(sizeof(EaAVPackageList));
   if(!pkt1)
       return -1;
   pkt1->pkt=*pkt;
   pkt1->next=NULL;
   if(pkt==&flushPkt){
       this->serial++;
   }
   pkt1->serial=this->serial;

   if(!this->lastPkt){
      this->firstPkt=pkt1;
   }else{
       this->lastPkt->next=pkt1;
   }
   this->lastPkt=pkt1;
   this->NBPackets++;
   this->size+=pkt1->pkt.size+sizeof(*pkt1);
   this->duration=pkt1->pkt.duration;
   //XXX:should duplicate packtet data in DV case
   this->cond->wakeOne();
   return 0;
}

int EaPacketQueue::put(AVPacket *pkt)
{
    int ret;
    this->mutex->lock();
    ret=putPrivate(pkt);
    this->mutex->unlock();

    if(pkt!=&flushPkt&&ret<0){
       av_packet_unref(pkt);
    }
    return ret;
}

int EaPacketQueue::putNullPacket(int streamIndex)
{
   AVPacket pkt1,*pkt=&pkt1;
   av_init_packet(pkt);
   pkt->data=NULL;
   pkt->size=0;
   pkt->stream_index=streamIndex;
   return put(pkt);
}

void EaPacketQueue::flush()
{
    EaAVPackageList*pkt,*pkt1;
    this->mutex->lock();
    for(pkt=this->firstPkt;pkt;pkt=pkt1){
       pkt1=pkt->next;
       av_packet_unref(&pkt->pkt);
       av_freep(&pkt);
    }
    this->lastPkt=NULL;
    this->firstPkt=MULL;
    this->size=0;
    this->duration=0;
    this->mutex->unlock();
}

void EaPacketQueue::abort()
{
    this->mutex->lock();
    this->abortRequest=1;
    this->cond->wakeOne();
    this->mutex->unlock();
}

void EaPacketQueue::start()
{
    this->mutex->lock();
    this->abortRequest=0;
    putPrivate(&flushPkt);
    this->mutex->unlock();
}

//return <0 if aborted,0 if no packet and >0 if packet
int EaPacketQueue::get(AVPacket *pkt, int block,int *serial)
{
   EaAVPackageList*pkt1;
   int ret;
   this->mutex->lock();
   for(;;){
      if(this->abortRequest){
         ret=-1;
         break;
      }
      pkt1=this->firstPkt;
      if(pkt1){
         this->firstPkt=pkt1->next;
          if(!this->firstPkt){
            this->lastPkt=NULL;
          }
          this->NBPackets--;
          this->size-=pkt1->pkt.size+sizeof(*pkt1);
          *this=pkt1->pkt;
          if(serial){
             *serial=pkt1->serial;
          }
          av_free(pkt1);
          ret=1;
          break;
      }else if(!block){
          ret =0;
      }else{
          this->cond->wait(mutex);
      }
   }

   this->mutex->unlock();
   return ret;
}


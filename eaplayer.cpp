#include "eaplayer.h"
#include "libavutil/common.h"
#include "libavutil/mem.h"

#define MIX_MAXVOLUME 128
EaPlayer::EaPlayer()
{

}

EaPlayer::EaPlayer(char *fileName, AVInputFormat *inputFormat)
{
    this->fileName=av_strdup(fileName);
    if(!this->fileName){
       goto fail;
    }
    this->pictQ=new EaFrameQueue(this->videoQ,VIDEO_PICTURE_QUEUE_SIZE,1);
    this->subpQ=new EaFrameQueue(this->subtitleQ,SUBPICTURE_QUEUE_SIZE,0);
    this->sampQ=new EaFrameQueue(this->audioQ,SAMPLE_QUEUE_SIZE,1);
    if(!this->pictQ||!this->subpQ||!sampQ){
       qDebug()<<"frameQueue init error";
       goto fail;
    }

    this->videoQ=new EaPacketQueue();
    this->audioQ=new EaPacketQueue();
    this->subtitleQ=new EaPacketQueue();
    if(!this->videoQ||this->audioQ||!this->subtitleQ){

       goto fail;
    }
    if(!this->continueReadThread=new QThread()){
       goto fail;
    }
    this->vidClk=new EaClock(this->videoQ.serial);
    this->audClk=new EaClock(this->audioQ.serial);
    this->extClk=new EaClock(this->extClk.serial);
    this->audioClockSerail=-1;
    if(startupVolume<0||startupVolume>100){
       av_log(NULL,AV_LOG_WARNING,"volume set error");
    }
    startupVolume=av_clip(startupVolume,0,100);
    startupVolume=av_clip(MIX_MAXVOLUME*startupVolume/100,0,MIX_MAXVOLUME);
    this->audioVolume=startupVolume;
    this->muted=0;
    this->avSyncType=avSyncType;
    this->moveToThread(this->readTid);
    if(!this->readTid){
       av_log(NULL,AV_LOG_FATAL,"create thread");
       delete *this;
    }
    connect(this,SIGNAL(resultReadThread(QString)),this,SLOT(readThreadWork(QString)));
    this->readTid->start();

    //this->readThread->quit();
    //this->readThread->wait();
}

EaPlayer::~EaPlayer()
{
    //xxx: use a special url_shutdown call to abort parse cleanly
    this->abortRequest=1;
    this->readTid->wait();

    //close each stream
    if(this->audioStream){
       streamComponentClose(this->audioStream);
    }
    if(this->videoStream>=0){
       streamComponentClose(this->videoStream);
    }
    if(this->subtitleStream>=0){
       streamComponentClose(this->subtitleStream);
    }
    av_format_close_input(&this->videoQ);
    delete this->videoQ;
    delete this->audioQ;
    delete this->subtitleQ;

    //free all pictures
    delete this->pictQ;
    delete this->sampQ;
    delete this->subpQ;

    QThread::destroyed(this->continueReadThread);
    sws_freeContext(this->imgConvertCtx);
    sws_freeContext(this->subConvetCtx);
    av_free(this->fileName);
    //destory texture

}

/**
 * @brief EaPlayer::readThreadWork
 *          read the stream
 * @param parameter
 */
void EaPlayer::readThreadWork(const QString &parameter)
{
    QString result;
    //执行线程操作

    //emit 避免循环
    emit resultReadThread(result);
}
void EaPlayer::streamComponentClose(int streamIndex)
{
   AVFormatContext* ic=this->ic;
   AVCodecParameters* codecPar;
   if(streamIndex<0||streamIndex>=ic->nb_streams){
      return;
   }
   codecPar=ic->streams[streamIndex]->codecpar;
   switch (codecPar->codec_type) {
   case AVMEDIA_TYPE_AUDIO:
       this->audDec.abort(this->sampQ);
       //关闭声音 todo

       delete this->audDec;
       swr_free(this->swsCtx);
       av_freep(this->audioBuf1);
       this->audioBuf1Size=0;
       this->audioBuf=NULL;
       if(this->rdft){
          av_rdft_end(this->rdft);
          av_freeq(this->rdftData);
          this->rdft=NULL;
          this->rdftBits=0;
       }
       break;
   case AVMEDIA_TYPE_VIDEO:
       this->vidDec.abort(this->pictQ);
       delete this->vidDec;
              break;
   case AVMEDIA_TYPE_SUBTITLE:
       this->subDec.abort(this->subpQ);
       delete this->subDec;
       break;
   default:
       break;
   }
   //St: stream
   ic->streams[streamIndex]->discard=AVDISCARD_ALL;
   switch (codecPar->codec_type) {
   case AVMEDIA_TYPE_AUDIO:
       this->audioSt=NULL;
       this->audioStream=-1;
       break;
   case AVMEDIA_TYPE_VIDEO:
       this->videoSt=NULL;
       this->videoStream=-1;
       break;
   case AVMEDIA_TYPE_SUBTITLE:
       this->subtitleSt=NULL;
       this->subtitleStream=-1;
       break;
   default:
       break;
   }
}


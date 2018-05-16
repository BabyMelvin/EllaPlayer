#include "eaplayer.h"
#include "libavutil/common.h"
#include "libavutil/mem.h"
#include "QThread"
#include <QTime>

#define MIX_MAXVOLUME 128
extern "C"{
    extern AVDictionary *format_opts, *codec_opts, *resample_opts;
    extern AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,AVDictionary *codec_opts);
}
#define INT64_MAX __INT64_MAX__
#define INT64_MIN __INT64_MIN__
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
 *         1. read the stream
 * @param parameter
 */
void EaPlayer::readThreadWork(const QString &parameter)
{
    QString result;
    //执行线程操作
    AVFormatContext *ic=NULL;
    int err,i,ret;
    int stIndex[AVMEDIA_TYPE_NB];
    AVPacket pkt,*pkt=&pkt1;
    int64_t streamStartTime;
    int pktInPlayeRange=0;
    AVDictionaryEntry *t;
    QMutex *waitMutex=new QMutex;
    int scanAllPmtsSet=0;

    if(!waitMutex){
       av_log(NULL,AV_LOG_FATAL,"create mutex in readThread");
       ret=AVERROR(ENOMEM);
       goto fail;
    }
    memset(stIndex,-1,sizeof(stIndex));
    this->lastVideoStream=this->videoStream=-1;
    this->lastAudioStream=this->audioStream=-1;
    this->lastSubtitleStream=this->subtitleStream=-1;
    this->eof=0;

    ic=avformat_alloc_context();
    if(!ic){
       av_log(NULL,AV_LOG_FATAL,"could not alloc context");
       ret=AVERROR(ENOMEM);
       goto fail;
    }
    ic->interrupt_callback.callback=decodeInterruptCb;
    ic->interrupt_callback.opaque=this;

    if(!av_dict_get(formatOpts,"scan_all_pmts",NULL,AV_DICT_MATCH_CASE)){
        av_dict_set(&formatOpts,"scan_all_pmts",1,AV_DICT_MATCH_CASE);
        scanAllPmtsSet=1;
    }
    err=avformat_open_input(&ic,this->fileName,this->iformat,&format_opts);
    if(!err){
        qDebug()<<"error open fileName:"<<fileName;
        ret=-1;
        goto fail;
    }
    if(scanAllPmtsSet){
        av_dict_set(&format_opts,"scan_all_ptms",NULL,AV_DICT_MATCH_CASE);
    }

    if((t=av_dict_get(format_opts,"",NULL,AV_DICT_IGNORE_SUFFIX))){
       av_log(NULL,AV_LOG_ERROR,"option % not found",t->key);
       ret=AVERROR_OPTION_NOT_FOUND;
       goto fail;
    }
    this->ic=ic;

    if(genpts){
       ic->flags|=AVFMT_FLAG_GENPTS;
    }

    av_format_inject_global_side_data(ic);
    if(findStreamInfo){
        AVDictionary** opts=setup_find_stream_info_opts(ic,codecOpts);
        int origNBStreams=ic->nb_streams;
        err=avformat_find_stream_info(ic,opts);
        for(i=0;i<origNBStreams;i++){
           av_dict_free(&opts[i]);
        }
        av_freep(&opts);
        if(err<0){
           av_log(NULL,AV_LOG_WARNING,"could not find codec: parameter",this->fileName);
           ret=-1;
           goto fail;
        }
    }
    if(ic->pb){
       //FIXME hack,ffplay maybe shoule not  use avio_feof() to test for the end
       ic->pb->eof_reached=0;
    }
    if(seekByBytes<0){
       seekByBytes=!!(ic->iformat->flags&AVFMT_TS_DISCONT)&&strcmp("ogg",ic->iformat->name);
    }
    this->maxFrameDuration=(ic->iformat->flags&AVFMT_TS_DISCONT)?10.0:3600.0;
    if(this->windowTile&&(t=av_dict_get(ic->metadata,"title",NULL,0)))
        this->windowTile=av_sprintf("%s-%s",t->value,fileName);
    //if seeking requested ,we excute it
    if(!startTime!=AV_NOPTS_VALUE){
       int64_t timeStamp;
       timeStamp=startTime;
       //add the stream start time
       if(ic->start_time!=AV_NOPTS_VALUE){
          timeStamp+=ic->start_time;
       }
       ret=avformat_seek_file(ic,-1,INT64_MIN,timeStamp,INT64_MAX,0);
       if(ret<0){
          av_log(NULL,AV_LOG_WARNING,"%s:could not seek to postion %0.3f",this->fileName,static_cast<double> timeStamp/AV_TIME_BASE);
       }
    }
    this->realtime=isRealtime(ic);
    if(showStatus){
       av_dump_format(ic,0,this->fileName,0);
    }

    for(i=0;i<ic->nb_streams;i++){
       AVStream*st=ic->streams[i];
       enum AVMediaType type=st->codecpar->codec_type;
       st->discard=AVDISCARD_ALL;
       if(type>=0&&wantedStreamSpec[type]&&stIndex[type]==-1){
          if(avformat_match_stream_specifier(ic,st,wantedStreamSpec)){
            stIndex[type] =i;
          }
       }
    }

    for(i=0;i<AVMEDIA_TYPE_NB;i++){
       if(wantedStreamSpec[i]&&stIndex[i]==-1){
          av_log(NULL,AV_LOG_ERROR,"stream specifer %s does not match any % stream\n",wantedStreamSpec[i],av_get_media_type_string(i));
          stIndex[i]=INT_MAX;
       }
    }

    if(!videoDisable){
       stIndex[AVMEDIA_TYPE_VIDOE]=av_find_best_stream(ic,AVMEDIA_TYPE_VIDEO,stIndex[AVMEDIA_TYPE_VIDEO],-1,NULL,0);
    }
    if(!audioDiable){
       stIndex[AVMEDIA_TYPE_AUDIO]=av_find_best_stream(ic,AVMEDIA_TYPE_AUDIO,stIndex[AVMEDIA_TYPE_AUDIO],stIndex[AVMEDIA_TYPE_VIDEO],NULL,0);
    }
    if(!videoDisable&&!subtitleDisable){
       stIndex[AVMEDIA_TYPE_AUDIO]=av_find_best_stream(ic,AVMEDIA_TYPE_SUBTITLE,stIndex[AVMEDIA_TYPE_SUBTITLE],stIndex[AVMEDIA_TYPE_AUDIO]>=0?stIndex[AVMEDIA_TYPE_AUDIO]:stIndex[AVMEDIA_TYPE_VIDEO],NULL,0);
    }
    this->showMode=showMode;
    if(stIndex[AVMEDIA_TYPE_VIDEO]>=0){
       AVStream *st=ic->streams[stIndex[AVMEDIA_TYPE_VIDEO]] ;
       AVCodecParameters *codecpar=st->codecpar;
       AVRational sar=av_guess_sample_aspect_ratio(ic,st,NULL);
       if(codecPar->width){
          //设置窗口尺寸 todo
           //codecpar->width,codecpar->height,sar
       }
    }
    //open stream
    if(stIndex[AVMEDIA_TYPE_AUDIO]>=0){
       streamComponentOpen(stIndex[AVMEDIA_TYPE_VIDEO]);
    }
    ret=-1;
    if(stIndex[AVMEDIA_TYPE_VIDEO]>=0){
       ret=streamComponentOpen(stIndex[AVMEDIA_TYPE_VIDEO]);
    }
    if(this->showMode==SHOW_MODE_NONE){
       this->showMode=ret>=0?SHOW_MODE_VIDOE:SHOW_MODE_RDFT;
    }
    if(stIndex[AVMEDIA_TYPE_SUBTITLE]>=0){
       streamComponentOpen(stIndex[AVMEDIA_TYPE_SUBTITLE]);
    }

    if(this->videoStream<0&&this->audioStream<0){
       av_log(NULL,AV_LOG_FATAL,"failed oopen the file %s or configure",fileName);
       ret=-1;
       goto fail;
    }
    if(infinite_buffer<0&&this->realtime){
       infinite_buffer=1;
    }
    for(;;){
       if(this->abortRequest){
          break;
       }
       if(this->paused!=this->lastPaused){
          this->lastPaused=this->paused;
           if(this->paused)
               this->readPauseReturn=av_read_pause(ic);
           else
               av_read_play(ic);
#if CONFIG_RTSP_DEMUTER || CONFIG_MMSH_PROTOCOL
           if(this->paused&&(!strmp(ic->iformat->name,"rtsp")||
                             (ic->pb&&!strncmp(input_filename,"mmsh:", 5))){
               //wait 10ms to avoid trying to get another packet
              //todo

               continue;
           }
#endif
       }
       if(this->seekReq){
          int64_t seekTarget=this->seekPos;
          int64_t seekMin=this->seekRel>0?seekTarget-this->seekRel+2:INT64_MIN;
          int64_t seekMax=this->seekRel>0?seekTarget-this->seekRel-2:INT64_MAX;
          //FIXME the +-2 due to rouding being not done int the corrent direction
          // of the seek_pos/seek_rel varaibales
          ret=avformat_seek_file(this-ic,-1,seekMin,seekTarget,seekMax,this->seekFlags);
          if(ret<0){
                   av_log(NULL,AV_LOG_ERROR,"%s error while seeking\n",this->ic->filename);
          }else{
              if(this->audioStream>=0){
                  audioQ.flush();
                  audioQ.put(EaPacketQueue::flushPkt);
              }
              if(this->subtitleStream>=0){
                   subtitleQ.flush();
                   subtitleQ.put(EaPacketQueue::flushPkt);
              }
              if(this->videoStream>=0){
                   videoQ.flush();
                   videoQ.put(EaPacketQueue::flushPkt);
              }
              if(this->seekFlags&AVSEEK_FLAG_BYTE){
                   extClk.setClock(NAN,0);
              }else{
                   extClk.setClock(seekTarget/(double)AV_TIME_BASE,0);
              }
              this->seekReq=0;
              this->queueAttachmentsReq=1;
              this->eof=0;
              if(this->paused){
                   stepNextFrame();
              }
          }
          if(this->queueAttachmentsReq){
             if(this->videoSt&&this->videoSt->disposition&AV_DISPOSITION_ATTACHED_PIC){
                AVPacket copy={0};
                if((ret=av_packet_ref(&copy,&this->videoSt->attached_pic))<0){
                   goto fail;
                }
                videoQ.put(&copy);
                videoQ.putNullPacket(this->videoStream);
             }
             this->queueAttachmentsReq=0;
          }
          //if the queue are full ,no need to read more
          if(infiniteBuffer<1&&(this->audioQ.size+this->videoQ.size+this->subtitleQ.size>MAX_QUEUE_SIZE
                                || (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq) &&
                                                stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq) &&
                                                stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, &is->subtitleq)))) {
               // wait 10 ms
               this->
          }
       }
    }
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

// todo
int EaPlayer::decodeInterruptCb(void *arg)
{

}

bool EaPlayer::isRealtime(AVFormatContext*s)
{
   if(!strcmp(s->iformat->name,"rtp")||
           !strcmp(s->iformat->name,"rtsp")||
           !strcmp(s->iformat->name,"sdp")){
        return true;
   }
   if(s->pb&&(!strncmp(s->filename,"rtp:",4)||strncmp(s->filename,"udp:",4))){
      return true;
   }
   return false;
}

int EaPlayer::streamComponentOpen(int stIndex)
{

}

void EaPlayer::stepNextFrame()
{

}


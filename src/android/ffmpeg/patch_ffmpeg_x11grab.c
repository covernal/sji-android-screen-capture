/*
 * android video grab interface
 * Copyright (C) 2006 Jin Qian <osexp2003@gmail.com>
 */

#include "config.h"
#include "libavformat/internal.h"
#include "libavutil/log.h"
#include "libavutil/opt.h"
#include "libavutil/parseutils.h"
#include "libavutil/time.h"
#include "libavutil/pixdesc.h"
#include <time.h>
#include "avdevice.h"
#include <dlfcn.h>
#include <unistd.h>
#include <stdbool.h>

struct ASC {
    void* priv_data;
    void* data;
    int size;
    int width;
    int height;
    char pixfmtName[32];
};

/**
 * Android Device Demuxer context
 */
struct androidgrab {
    const AVClass *class;    /**< Class for private options. */

    // double framerate;

    void* dlhandle;
    void (*asc_capture)(struct ASC*);
    struct ASC asc;
    bool haveRestDataInLastCall;
    bool forProbe;

    // int interval; //unit: us
    // int64_t time_frame;      /**< Current time */
};

/**
 * Initialize the android grab device demuxer (public device demuxer API).
 *
 * @param s1 Context from avformat core
 * @return <ul>
 *          <li>AVERROR(ENOMEM) no memory left</li>
 *          <li>AVERROR(EIO) other failure case</li>
 *          <li>0 success</li>
 *         </ul>
 */
static int
androidgrab_read_header(AVFormatContext *s1)
{
    struct androidgrab *agrab = s1->priv_data;
    AVStream *st = NULL;

    st = avformat_new_stream(s1, NULL);
    if (!st) return AVERROR(ENOMEM);

    //avpriv_set_pts_info(st, 64, 1, 1000000); /* 64 bits pts in us */

    // agrab->time_frame = av_gettime();
    // agrab->interval = ((double)1000000)/agrab->framerate;
    agrab->dlhandle = dlopen(s1->filename, RTLD_NOW);
    if (!agrab->dlhandle) {
        av_log(s1, AV_LOG_ERROR, "dlopen err:%d(%s) %s\n", errno, strerror(errno), dlerror());
        return AVERROR(errno);
    }

    agrab->asc_capture = dlsym(agrab->dlhandle, "asc_capture");
    if (!agrab->asc_capture) {
        av_log(s1, AV_LOG_ERROR, "dlsym err:%d(%s)\n", errno, strerror(errno));
        return AVERROR(errno);
    }

    agrab->asc.priv_data = NULL;
    agrab->asc_capture(&agrab->asc);

    agrab->haveRestDataInLastCall = true;
    agrab->forProbe = true;

    st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codec->codec_id = AV_CODEC_ID_RAWVIDEO;
    st->codec->width  = agrab->asc.width;
    st->codec->height = agrab->asc.height;
    st->codec->pix_fmt = av_get_pix_fmt(agrab->asc.pixfmtName);
    // st->codec->time_base = av_d2q(1/agrab->framerate, 1000000);
    // st->codec->bit_rate = agrab->asc.size * agrab->framerate * 8;

    return 0;
}

/**
 * Grab a frame from Android (public device demuxer API).
 *
 * @param s1 Context from avformat core
 * @param pkt Packet holding the grabbed frame
 * @return frame size in bytes
 */
static int
androidgrab_read_packet(AVFormatContext *s1, AVPacket *pkt)
{
    struct androidgrab *agrab = s1->priv_data;
    // int64_t curtime;
    // int64_t waitInterval;
    // int ret;

    // /* Calculate the time of the next frame */
    // agrab->time_frame += agrab->interval;

    // /* wait based on the frame rate */
    // for(;;) {
    //     curtime = av_gettime();
    //     waitInterval = agrab->time_frame - curtime;
    //     if (waitInterval <= 0) {
    //         agrab->time_frame = curtime;
    //         break;
    //     }
    //     usleep(waitInterval);
    // }

    av_init_packet(pkt);
    // pkt->pts = curtime;

    if (agrab->haveRestDataInLastCall) {
        if (agrab->forProbe) {
            agrab->forProbe = false;
        } else {
            agrab->haveRestDataInLastCall = false;
        }
    } else {
        agrab->asc_capture(&agrab->asc);
    }
    pkt->data = agrab->asc.data;
    pkt->size = agrab->asc.size;

    return agrab->asc.size;
}

/**
 * Close Android frame grabber (public device demuxer API).
 *
 * @param s1 Context from avformat core
 * @return 0 success, !0 failure
 */
static int
androidgrab_read_close(AVFormatContext *s1)
{
    return 0;
}

#define OFFSET(x) offsetof(struct androidgrab, x)
#define DEC AV_OPT_FLAG_DECODING_PARAM
static const AVOption options[] = {
    // { "framerate", "set video frame rate",  OFFSET(framerate),  AV_OPT_TYPE_DOUBLE, {.dbl = 4}, 0.1, 40,   DEC },
    { "width",     "horizontal size",   OFFSET(asc.width),  AV_OPT_TYPE_INT,    {.i64 = 0}, 0,   4096, DEC },
    { "height",    "vertical size",     OFFSET(asc.height), AV_OPT_TYPE_INT,    {.i64 = 0}, 0,   4096, DEC },
    { NULL },
};

static const AVClass android_class = {
    .class_name = "androidgrab indev",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

/** android grabber device demuxer declaration */
AVInputFormat ff_x11grab_demuxer = {
    .name           = "androidgrab",
    .long_name      = NULL_IF_CONFIG_SMALL("androidgrab"),
    .priv_data_size = sizeof(struct androidgrab),
    .read_header    = androidgrab_read_header,
    .read_packet    = androidgrab_read_packet,
    .read_close     = androidgrab_read_close,
    .flags          = AVFMT_NOFILE,
    .priv_class     = &android_class,
};

/*
 * Blackmagic Devices Decklink capture
 * Copyright (c) 2013 Luca Barbato.
 *
 * This file is part of libbmd.
 *
 * libbmd is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libbmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libbmd; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include <libavformat/avformat.h>
#include "decklink_capture.h"

pthread_mutex_t sleepMutex;
pthread_cond_t sleepCond;
int videoOutputFile = -1;
int audioOutputFile = -1;

static int g_videoModeIndex      = -1;
static int g_audioChannels       = 2;
static int g_audioSampleDepth    = 16;
const char *g_videoOutputFile    = NULL;
const char *g_audioOutputFile    = NULL;
static int g_maxFrames           = -1;
int g_verbose                    = 0;
unsigned long long g_memoryLimit = 1024 * 1024 * 1024;            // 1GByte(>50 sec)

static unsigned long frameCount = 0;
static unsigned int dropped     = 0, totaldropped = 0;
static enum PixelFormat pix_fmt = PIX_FMT_UYVY422;
static enum AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16;

typedef struct AVPacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    unsigned long long size;
    int abort_request;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} AVPacketQueue;

static AVPacketQueue queue;

static AVPacket flush_pkt;

static void avpacket_queue_init(AVPacketQueue *q)
{
    memset(q, 0, sizeof(AVPacketQueue));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

static void avpacket_queue_flush(AVPacketQueue *q)
{
    AVPacketList *pkt, *pkt1;

    pthread_mutex_lock(&q->mutex);
    for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
        pkt1 = pkt->next;
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt   = NULL;
    q->first_pkt  = NULL;
    q->nb_packets = 0;
    q->size       = 0;
    pthread_mutex_unlock(&q->mutex);
}

static void avpacket_queue_end(AVPacketQueue *q)
{
    avpacket_queue_flush(q);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

static int avpacket_queue_put(AVPacketQueue *q, AVPacket *pkt)
{
    AVPacketList *pkt1;

    /* duplicate the packet */
    if (pkt != &flush_pkt && av_dup_packet(pkt) < 0) {
        return -1;
    }

    pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));
    if (!pkt1) {
        return -1;
    }
    pkt1->pkt  = *pkt;
    pkt1->next = NULL;

    pthread_mutex_lock(&q->mutex);

    if (!q->last_pkt) {
        q->first_pkt = pkt1;
    } else {
        q->last_pkt->next = pkt1;
    }

    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);

    pthread_cond_signal(&q->cond);

    pthread_mutex_unlock(&q->mutex);
    return 0;
}

static int avpacket_queue_get(AVPacketQueue *q, AVPacket *pkt, int block)
{
    AVPacketList *pkt1;
    int ret;

    pthread_mutex_lock(&q->mutex);

    for (;; ) {
        pkt1 = q->first_pkt;
        if (pkt1) {
            if (pkt1->pkt.data == flush_pkt.data) {
                ret = 0;
                break;
            }
            q->first_pkt = pkt1->next;
            if (!q->first_pkt) {
                q->last_pkt = NULL;
            }
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            *pkt     = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&q->cond, &q->mutex);
        }
    }
    pthread_mutex_unlock(&q->mutex);
    return ret;
}

static unsigned long long avpacket_queue_size(AVPacketQueue *q)
{
    unsigned long long size;
    pthread_mutex_lock(&q->mutex);
    size = q->size;
    pthread_mutex_unlock(&q->mutex);
    return size;
}

AVFrame *picture;
AVOutputFormat *fmt = NULL;
AVFormatContext *oc;
AVStream *audio_st, *video_st;

static AVStream *add_audio_stream(AVFormatContext *oc, enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVCodec *codec;
    AVStream *st;

    st = avformat_new_stream(oc, NULL);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        exit(1);
    }

    c             = st->codec;
    c->codec_id   = codec_id;
    c->codec_type = AVMEDIA_TYPE_AUDIO;

    /* put sample parameters */
    c->sample_fmt = sample_fmt;
//    c->bit_rate = 64000;
    c->sample_rate = 48000;
    c->channels    = g_audioChannels;
    // some formats want stream headers to be separate
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    return st;
}

static AVStream *add_video_stream(DecklinkConf *conf,
                                  AVFormatContext *oc, enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVCodec *codec;
    AVStream *st;

    st = avformat_new_stream(oc, NULL);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        exit(1);
    }

    c             = st->codec;
    c->codec_id   = codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;

    /* put sample parameters */
//    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width  = conf->width;
    c->height = conf->height;
    /* time base: this is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. for fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identically 1.*/
    c->time_base.den = conf->tb_den;
    c->time_base.num = conf->tb_num;
    c->pix_fmt       = pix_fmt;

    if (codec_id == AV_CODEC_ID_V210)
        c->bits_per_raw_sample = 10;
    // some formats want stream headers to be separate
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    /* open the codec */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }
    picture = avcodec_alloc_frame();

    return st;
}


static int video_callback(void *priv, uint8_t *frame,
                          int width, int height, int stride,
                          int64_t timestamp, int64_t duration)
{
//    CaptureContext *ctx = priv;
    AVPacket pkt;
    AVCodecContext *c;
    av_init_packet(&pkt);
    c = video_st->codec;
    if (g_verbose && frameCount % 25 == 0) {
        uint64_t qsize = avpacket_queue_size(&queue);
        fprintf(stderr,
                "Frame received (#%lu) - Valid (%dB) - QSize %f\n",
                frameCount,
                stride * height,
                (double)qsize / 1024 / 1024);
    }
    avpicture_fill((AVPicture *)picture, (uint8_t *)frame,
                   pix_fmt,
                   width, height);

    pkt.pts      = pkt.dts = timestamp / video_st->time_base.num;
    pkt.duration = duration / video_st->time_base.num;
    //To be made sure it still applies
    pkt.flags       |= AV_PKT_FLAG_KEY;
    pkt.stream_index = video_st->index;
    pkt.data         = frame;
    pkt.size         = stride * height;
    c->frame_number++;
    avpacket_queue_put(&queue, &pkt);

    return 0;
}

static int audio_callback(void *priv, uint8_t *frame,
                          int nb_samples,
                          int64_t timestamp)
{
//    CaptureContext *ctx = priv;
    AVCodecContext *c;
    AVPacket pkt;
    av_init_packet(&pkt);

    c = audio_st->codec;
    //hack among hacks
    pkt.size = nb_samples * g_audioChannels * (g_audioSampleDepth / 8);
    pkt.dts = pkt.pts = timestamp / audio_st->time_base.num;
    pkt.flags       |= AV_PKT_FLAG_KEY;
    pkt.stream_index = audio_st->index;
    pkt.data         = frame;
    c->frame_number++;
    avpacket_queue_put(&queue, &pkt);

    return 0;
}

static void *push_packet(void *ctx)
{
    AVFormatContext *s = (AVFormatContext *)ctx;
    AVPacket pkt;
    int ret;

    while (avpacket_queue_get(&queue, &pkt, 1)) {
        pkt.destruct = NULL;
        av_interleaved_write_frame(s, &pkt);
        av_destruct_packet(&pkt);
        av_free_packet(&pkt);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int displayModeCount               = 0;
    int exitStatus                     = 1;
    int ch, i;
    DecklinkConf c  = { .video_cb = video_callback,
                        .audio_cb = audio_callback };
    DecklinkCapture *capture;
    pthread_t th;

    pthread_mutex_init(&sleepMutex, NULL);
    pthread_cond_init(&sleepCond, NULL);
    av_register_all();

    // Parse command line options
    while ((ch = getopt(argc, argv, "?hvc:s:f:a:m:n:p:M:F:C:A:V:")) != -1) {
        switch (ch) {
        case 'v':
            g_verbose = 1;
            break;
        case 'm':
            c.video_mode = atoi(optarg);
            break;
        case 'c':
            c.audio_channels = atoi(optarg);
            break;
        case 's':
            c.audio_sample_depth = atoi(optarg);
            switch (c.audio_sample_depth) {
            case 16:
                sample_fmt = AV_SAMPLE_FMT_S16;
                break;
            case 32:
                sample_fmt = AV_SAMPLE_FMT_S32;
                break;
            default:
                goto bail;
            }
            break;
        case 'p':
            switch (atoi(optarg)) {
            case  8:
                c.pixel_format = 0;
                pix_fmt = AV_PIX_FMT_UYVY422;
                break;
            case 10:
                c.pixel_format = 1;
                pix_fmt = AV_PIX_FMT_YUV422P10;
                break;
            default:
                fprintf(
                    stderr,
                    "Invalid argument: Pixel Format Depth must be either 8 bits or 10 bits\n");
                goto bail;
            }
            break;
        case 'f':
            g_videoOutputFile = optarg;
            break;
        case 'n':
            g_maxFrames = atoi(optarg);
            break;
        case 'M':
            g_memoryLimit = atoi(optarg) * 1024 * 1024 * 1024L;
            break;
        case 'F':
            fmt = av_guess_format(optarg, NULL, NULL);
            break;
        case 'A':
            c.audio_connection = atoi(optarg);
            break;
        case 'V':
            c.video_connection = atoi(optarg);
            break;
        case 'C':
            c.instance = atoi(optarg);
            break;
        case '?':
        case 'h':
            exit(0);
        }
    }

    capture = decklink_capture_alloc(&c);

    if (!g_videoOutputFile) {
        fprintf(stderr,
                "Missing argument: Please specify output path using -f\n");
        goto bail;
    }

    if (!fmt) {
        fmt = av_guess_format(NULL, g_videoOutputFile, NULL);
        if (!fmt) {
            fprintf(
                stderr,
                "Unable to guess output format, please specify explicitly using -F\n");
            goto bail;
        }
    }

    oc          = avformat_alloc_context();
    oc->oformat = fmt;

    snprintf(oc->filename, sizeof(oc->filename), "%s", g_videoOutputFile);

    fmt->video_codec = (c.pixel_format == 0 ? AV_CODEC_ID_RAWVIDEO : AV_CODEC_ID_V210);
    fmt->audio_codec = (sample_fmt == AV_SAMPLE_FMT_S16 ? AV_CODEC_ID_PCM_S16LE : AV_CODEC_ID_PCM_S32LE);

    video_st = add_video_stream(&c, oc, fmt->video_codec);
    audio_st = add_audio_stream(oc, fmt->audio_codec);

    if (!(fmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&oc->pb, oc->filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open '%s'\n", oc->filename);
            exit(1);
        }
    }

    avformat_write_header(oc, NULL);


    avpacket_queue_init(&queue);

    if (pthread_create(&th, NULL, push_packet, oc))
        goto bail;

    decklink_capture_start(capture);

    // Block main thread until signal occurs
    pthread_mutex_lock(&sleepMutex);
    pthread_cond_wait(&sleepCond, &sleepMutex);
    pthread_mutex_unlock(&sleepMutex);
    fprintf(stderr, "Stopping Capture\n");

    decklink_capture_stop(capture);

bail:

    decklink_capture_free(capture);

    if (oc != NULL) {
        av_write_trailer(oc);
        if (!(fmt->flags & AVFMT_NOFILE)) {
            /* close the output file */
            avio_close(oc->pb);
        }
    }

    return exitStatus;
}

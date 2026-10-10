/* Imhotep - MP3 and Bink decoding through FFmpeg (see media.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "platform/media.h"

#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

struct media {
    AVFormatContext *fmt;
    AVCodecContext *audio, *video;
    int ai, vi;
    SwrContext *swr;
    struct SwsContext *sws;
    AVPacket *pkt;
    AVFrame *frame;
    int16_t *pcm; /* stereo frames, [start, count) not taken */
    int start, count, cap;
    uint8_t *rgb;
    int w, h;
    int64_t frame_ms;
    int eof_sent;
};

static AVCodecContext *open_codec(AVFormatContext *fmt, int index)
{
    const AVCodecParameters *par = fmt->streams[index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(par->codec_id);
    if (!codec) {
        return NULL;
    }
    AVCodecContext *c = avcodec_alloc_context3(codec);
    if (!c || avcodec_parameters_to_context(c, par) < 0 || avcodec_open2(c, codec, NULL) < 0) {
        avcodec_free_context(&c);
        return NULL;
    }
    return c;
}

media *media_open(const char *path, int video)
{
    media *m = calloc(1, sizeof(media));
    if (!m) {
        return NULL;
    }
    m->ai = m->vi = -1;
    av_log_set_level(AV_LOG_ERROR);
    if (avformat_open_input(&m->fmt, path, NULL, NULL) < 0 || avformat_find_stream_info(m->fmt, NULL) < 0) {
        media_close(m);
        return NULL;
    }
    m->ai = av_find_best_stream(m->fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (m->ai >= 0) {
        m->audio = open_codec(m->fmt, m->ai);
        if (!m->audio) {
            m->ai = -1;
        }
    }
    if (video) {
        m->vi = av_find_best_stream(m->fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (m->vi >= 0) {
            m->video = open_codec(m->fmt, m->vi);
            if (!m->video) {
                m->vi = -1;
            }
        }
    }
    if ((video && m->vi < 0) || (!video && m->ai < 0)) {
        media_close(m);
        return NULL;
    }
    if (m->audio) {
#if LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(4, 5, 100)
        AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
        if (swr_alloc_set_opts2(&m->swr, &stereo, AV_SAMPLE_FMT_S16, MEDIA_RATE, &m->audio->ch_layout,
                                m->audio->sample_fmt, m->audio->sample_rate, 0, NULL) < 0 ||
            swr_init(m->swr) < 0) {
#else
        /* FFmpeg before 5.1 (Ubuntu 22.04's 4.4, which the Linux release is built against): the
         * channel layout as a mask */
        int64_t layout = m->audio->channel_layout ? (int64_t) m->audio->channel_layout
                                                  : av_get_default_channel_layout(m->audio->channels);
        m->swr = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, MEDIA_RATE, layout,
                                    m->audio->sample_fmt, m->audio->sample_rate, 0, NULL);
        if (!m->swr || swr_init(m->swr) < 0) {
#endif
            media_close(m);
            return NULL;
        }
    }
    if (m->video) {
        m->w = m->video->width;
        m->h = m->video->height;
        m->rgb = malloc((size_t) m->w * m->h * 4);
        if (!m->rgb) {
            media_close(m);
            return NULL;
        }
    }
    m->pkt = av_packet_alloc();
    m->frame = av_frame_alloc();
    if (!m->pkt || !m->frame) {
        media_close(m);
        return NULL;
    }
    return m;
}

void media_close(media *m)
{
    if (!m) {
        return;
    }
    avcodec_free_context(&m->audio);
    avcodec_free_context(&m->video);
    swr_free(&m->swr);
    sws_freeContext(m->sws);
    av_packet_free(&m->pkt);
    av_frame_free(&m->frame);
    avformat_close_input(&m->fmt);
    free(m->pcm);
    free(m->rgb);
    free(m);
}

static int reserve(media *m, int frames)
{
    if (m->start > 0 && m->start + m->count + frames > m->cap) {
        memmove(m->pcm, m->pcm + 2 * m->start, (size_t) m->count * 4);
        m->start = 0;
    }
    if (m->count + frames > m->cap) {
        int cap = m->cap ? m->cap * 2 : 65536;
        while (cap < m->count + frames) {
            cap *= 2;
        }
        int16_t *p = realloc(m->pcm, (size_t) cap * 4);
        if (!p) {
            return -1;
        }
        m->pcm = p;
        m->cap = cap;
    }
    return 0;
}

static int audio_frames(media *m)
{
    int got = 0;
    while (avcodec_receive_frame(m->audio, m->frame) == 0) {
        int out = swr_get_out_samples(m->swr, m->frame->nb_samples);
        if (out > 0 && reserve(m, out) == 0) {
            uint8_t *dst = (uint8_t *) (m->pcm + 2 * (m->start + m->count));
            int n = swr_convert(m->swr, &dst, out, (const uint8_t **) m->frame->extended_data, m->frame->nb_samples);
            if (n > 0) {
                m->count += n;
                got = 1;
            }
        }
        av_frame_unref(m->frame);
    }
    return got;
}

static int video_frame(media *m)
{
    int got = 0;
    while (avcodec_receive_frame(m->video, m->frame) == 0) {
        m->sws = sws_getCachedContext(m->sws, m->frame->width, m->frame->height, m->frame->format, m->w, m->h,
                                      AV_PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
        if (m->sws) {
            uint8_t *dst[4] = {m->rgb};
            int pitch[4] = {m->w * 4};
            sws_scale(m->sws, (const uint8_t *const *) m->frame->data, m->frame->linesize, 0, m->frame->height, dst, pitch);
            AVRational tb = m->fmt->streams[m->vi]->time_base;
            int64_t pts = m->frame->best_effort_timestamp;
            m->frame_ms = pts == AV_NOPTS_VALUE ? 0 : av_rescale_q(pts, tb, (AVRational){1, 1000});
            got = 1;
        }
        av_frame_unref(m->frame);
        if (got) {
            break;
        }
    }
    return got;
}

int media_step(media *m)
{
    if (m->video && video_frame(m)) {
        return 2; /* a frame left in the decoder */
    }
    int r = av_read_frame(m->fmt, m->pkt);
    if (r < 0) {
        if (!m->eof_sent) { /* drain the decoders */
            m->eof_sent = 1;
            int a = 0, v = 0;
            if (m->audio) {
                avcodec_send_packet(m->audio, NULL);
                a = audio_frames(m);
            }
            if (m->video) {
                avcodec_send_packet(m->video, NULL);
                v = video_frame(m);
            }
            return v ? 2 : a ? 1 : 0;
        }
        return 0;
    }
    int out = 3;
    if (m->pkt->stream_index == m->ai && m->audio) {
        if (avcodec_send_packet(m->audio, m->pkt) == 0 && audio_frames(m)) {
            out = 1;
        }
    } else if (m->pkt->stream_index == m->vi && m->video) {
        if (avcodec_send_packet(m->video, m->pkt) == 0 && video_frame(m)) {
            out = 2;
        }
    }
    av_packet_unref(m->pkt);
    return out;
}

int media_take(media *m, int16_t *out, int max)
{
    int n = m->count < max ? m->count : max;
    memcpy(out, m->pcm + 2 * m->start, (size_t) n * 4);
    m->start += n;
    m->count -= n;
    if (m->count == 0) {
        m->start = 0;
    }
    return n;
}

int media_pending(const media *m)
{
    return m->count;
}

int media_rewind(media *m)
{
    if (av_seek_frame(m->fmt, -1, 0, AVSEEK_FLAG_BACKWARD) < 0) {
        return -1;
    }
    if (m->audio) {
        avcodec_flush_buffers(m->audio);
    }
    if (m->video) {
        avcodec_flush_buffers(m->video);
    }
    m->eof_sent = 0;
    return 0;
}

int media_width(const media *m)
{
    return m->w;
}

int media_height(const media *m)
{
    return m->h;
}

const uint8_t *media_frame(const media *m)
{
    return m->rgb;
}

int64_t media_frame_ms(const media *m)
{
    return m->frame_ms;
}

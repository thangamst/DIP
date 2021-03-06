/*
 * openhevc.c wrapper to openhevc or ffmpeg
 * Copyright (c) 2012-2013 Micka�l Raulet, Wassim Hamidouche, Gildas Cocherel, Pierre Edouard Lepere
 *
 * This file is part of openhevc.
 *
 * openHevc is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * openhevc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with openhevc; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <stdio.h>
#include "openHevcWrapper.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"

//TMP
#include "libavcodec/h264.h"

#define MAX_DECODERS 3
#define ACTIVE_NAL
typedef struct OpenHevcWrapperContext {
    AVCodec *codec;
    AVCodecContext *c;
    AVFrame *picture;
    AVPacket avpkt;
    AVCodecParserContext *parser;
} OpenHevcWrapperContext;

typedef struct OpenHevcWrapperContexts {
    OpenHevcWrapperContext **wraper;
    int nb_decoders;
    int active_layer;
    int display_layer;
    int set_display;
    int set_vps;
} OpenHevcWrapperContexts;

SliceTypeDecodeCallback OpenHevcSliceTypeDecodeCallback = NULL;

OpenHevc_Handle libOpenHevcInit(int nb_pthreads, int thread_type)
{
    /* register all the codecs */
    int i;
    OpenHevcWrapperContexts *openHevcContexts = av_mallocz(sizeof(OpenHevcWrapperContexts));
    OpenHevcWrapperContext  *openHevcContext;
    avcodec_register_all();
    openHevcContexts->nb_decoders   = MAX_DECODERS;
    openHevcContexts->active_layer  = MAX_DECODERS-1;
    openHevcContexts->display_layer = MAX_DECODERS-1;
    openHevcContexts->wraper = av_malloc(sizeof(OpenHevcWrapperContext*)*openHevcContexts->nb_decoders);
    for(i=0; i < openHevcContexts->nb_decoders; i++){
        openHevcContext = openHevcContexts->wraper[i] = av_malloc(sizeof(OpenHevcWrapperContext));
        av_init_packet(&openHevcContext->avpkt);
        openHevcContext->codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
        if (!openHevcContext->codec) {
            fprintf(stderr, "codec not found\n");
            return NULL;
        }

        openHevcContext->parser  = av_parser_init( openHevcContext->codec->id );
        openHevcContext->c       = avcodec_alloc_context3(openHevcContext->codec);
        openHevcContext->picture = avcodec_alloc_frame();
        openHevcContext->c->flags |= AV_CODEC_FLAG_UNALIGNED;

        if(openHevcContext->codec->capabilities&AV_CODEC_CAP_TRUNCATED)
            openHevcContext->c->flags |= AV_CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

        /* For some codecs, such as msmpeg4 and mpeg4, width and height
         MUST be initialized there because this information is not
         available in the bitstream. */

        /*      set thread parameters    */
        if(thread_type == 1)
            av_opt_set(openHevcContext->c, "thread_type", "frame", 0);
        else if (thread_type == 2)
            av_opt_set(openHevcContext->c, "thread_type", "slice", 0);
        else
            av_opt_set(openHevcContext->c, "thread_type", "frameslice", 0);

        av_opt_set_int(openHevcContext->c, "threads", nb_pthreads, 0);

        /*  Set the decoder id    */
        av_opt_set_int(openHevcContext->c->priv_data, "decoder-id", i, 0);
    }
    return (OpenHevc_Handle) openHevcContexts;
}

OpenHevc_Handle libOpenH264Init(int nb_pthreads, int thread_type)
{
    /* register all the codecs */
    int i;
    OpenHevcWrapperContexts *openHevcContexts = av_mallocz(sizeof(OpenHevcWrapperContexts));
    OpenHevcWrapperContext  *openHevcContext;
    avcodec_register_all();
    openHevcContexts->nb_decoders   = MAX_DECODERS;
    openHevcContexts->active_layer  = MAX_DECODERS-1;
    openHevcContexts->display_layer = MAX_DECODERS-1;
    openHevcContexts->wraper = av_malloc(sizeof(OpenHevcWrapperContext*)*openHevcContexts->nb_decoders);
    for(i=0; i < openHevcContexts->nb_decoders; i++){
        openHevcContext = openHevcContexts->wraper[i] = av_malloc(sizeof(OpenHevcWrapperContext));
        av_init_packet(&openHevcContext->avpkt);
        openHevcContext->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!openHevcContext->codec) {
            fprintf(stderr, "codec not found\n");
            return NULL;
        }

        openHevcContext->parser  = av_parser_init( openHevcContext->codec->id );
        openHevcContext->c       = avcodec_alloc_context3(openHevcContext->codec);
        openHevcContext->picture = av_frame_alloc();
        openHevcContext->c->flags |= AV_CODEC_FLAG_UNALIGNED;

        if(openHevcContext->codec->capabilities&AV_CODEC_CAP_TRUNCATED)
            openHevcContext->c->flags |= AV_CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

        /* For some codecs, such as msmpeg4 and mpeg4, width and height
         MUST be initialized there because this information is not
         available in the bitstream. */

        /*      set thread parameters    */
        if(thread_type == 1)
            av_opt_set(openHevcContext->c, "thread_type", "frame", 0);
        else if (thread_type == 2)
            av_opt_set(openHevcContext->c, "thread_type", "slice", 0);
        else
            av_opt_set(openHevcContext->c, "thread_type", "frameslice", 0);

        av_opt_set_int(openHevcContext->c, "threads", nb_pthreads, 0);

        /*  Set the decoder id    */
        av_opt_set_int(openHevcContext->c->priv_data, "decoder-id", i, 0);
    }
    return (OpenHevc_Handle) openHevcContexts;
}
/**
 * Init up to MAX_DECODERS decoders for SHVC decoding in case of AVC Base Layer and
 * allocate their contexts
 *    -First decoder will be h264 decoder
 *    -Second one will be HEVC decoder
 *    -Third decoder is allocated but still unused since its not supported yet
 */
OpenHevc_Handle libOpenShvcInit(int nb_pthreads, int thread_type)
{
    /* register all the codecs */
    int i;
    OpenHevcWrapperContexts *openHevcContexts = av_mallocz(sizeof(OpenHevcWrapperContexts));
    OpenHevcWrapperContext  *openHevcContext;
    avcodec_register_all();
    openHevcContexts->nb_decoders   = MAX_DECODERS;
    openHevcContexts->active_layer  = MAX_DECODERS-1;
    openHevcContexts->display_layer = MAX_DECODERS-1;
    openHevcContexts->wraper = av_malloc(sizeof(OpenHevcWrapperContext*)*openHevcContexts->nb_decoders);
    for(i=0; i < openHevcContexts->nb_decoders; i++){
        openHevcContext = openHevcContexts->wraper[i] = av_malloc(sizeof(OpenHevcWrapperContext));
        av_init_packet(&openHevcContext->avpkt);
        if(i == 0)
        	openHevcContext->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        else
            openHevcContext->codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
        if (!openHevcContext->codec) {
            fprintf(stderr, "codec not found\n");
            return NULL;
        }

        openHevcContext->parser  = av_parser_init( openHevcContext->codec->id );
        openHevcContext->c       = avcodec_alloc_context3(openHevcContext->codec);
        openHevcContext->picture = av_frame_alloc();
        openHevcContext->c->flags |= AV_CODEC_FLAG_UNALIGNED;

        if(openHevcContext->codec->capabilities&AV_CODEC_CAP_TRUNCATED)
            openHevcContext->c->flags |= AV_CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

        /* For some codecs, such as msmpeg4 and mpeg4, width and height
         MUST be initialized there because this information is not
         available in the bitstream. */

        /*      set thread parameters    */
        if(thread_type == 1)
            av_opt_set(openHevcContext->c, "thread_type", "frame", 0);
        else if (thread_type == 2)
            av_opt_set(openHevcContext->c, "thread_type", "slice", 0);
        else
            av_opt_set(openHevcContext->c, "thread_type", "frameslice", 0);

        av_opt_set_int(openHevcContext->c, "threads", nb_pthreads, 0);

        /*  Set the decoder id    */
        av_opt_set_int(openHevcContext->c->priv_data, "decoder-id", i, 0);
    }
    return (OpenHevc_Handle) openHevcContexts;
}

void libOpenHevcOnSliceTypeDecodeCallback(SliceTypeDecodeCallback callback){
	OpenHevcSliceTypeDecodeCallback = callback;
}

int libOpenHevcStartDecoder(OpenHevc_Handle openHevcHandle)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;
    int i;
    for(i=0; i < openHevcContexts->nb_decoders; i++) {
        openHevcContext = openHevcContexts->wraper[i];
        if (avcodec_open2(openHevcContext->c, openHevcContext->codec, NULL) < 0) {
            fprintf(stderr, "could not open codec\n");
            return -1;
        }
        if(i+1 < openHevcContexts->nb_decoders)
            openHevcContexts->wraper[i+1]->c->BL_avcontext = openHevcContexts->wraper[i]->c;
    }
    return 1;
}

int libOpenHevcDecode(OpenHevc_Handle openHevcHandle, const unsigned char *buff, int au_len, int64_t pts)
{
    int got_picture[MAX_DECODERS], len=0, i, max_layer;
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;
    for(i =0; i < MAX_DECODERS; i++)  {
        got_picture[i]                 = 0;
        openHevcContext                = openHevcContexts->wraper[i];
        openHevcContext->c->quality_id = openHevcContexts->active_layer;
//        printf("quality_id %d \n", openHevcContext->c->quality_id);
        if (i <= openHevcContexts->active_layer) {
            openHevcContext->avpkt.size = au_len;
            openHevcContext->avpkt.data = (uint8_t *) buff;
        } else {
            openHevcContext->avpkt.size = 0;
            openHevcContext->avpkt.data = NULL;
        }
        openHevcContext->avpkt.pts  = pts;
        len                         = avcodec_decode_video2( openHevcContext->c, openHevcContext->picture,
                                                             &got_picture[i], &openHevcContext->avpkt);
        if(i+1 < openHevcContexts->nb_decoders)
            openHevcContexts->wraper[i+1]->c->BL_frame = openHevcContexts->wraper[i]->c->BL_frame;
    }
    if (len < 0) {
        fprintf(stderr, "Error while decoding frame \n");
        return -1;
    }
    if(openHevcContexts->set_display)
        max_layer = openHevcContexts->display_layer;
    else
        max_layer = openHevcContexts->active_layer;

    for(i=max_layer; i>=0; i--) {
        if(got_picture[i]){
            if(i == openHevcContexts->display_layer) {
                if (i >= 0 && i < openHevcContexts->nb_decoders)
                    openHevcContexts->display_layer = i;
                return got_picture[i];
            }
         //   fprintf(stderr, "Display layer %d  \n", i);

        }

    }
    return 0;
}

/**
 * Pass the packets to the corresponding decoders and loop over running decoders untill one of them
 * output a got_picture.
 *    -First decoder will be h264 decoder
 *    -Second one will be HEVC decoder
 *    -Third decoder is ignored since its not supported yet
 */
int libOpenShvcDecode(OpenHevc_Handle openHevcHandle, const AVPacket packet[], const int stop_dec1, const int stop_dec2)
{
    int got_picture[MAX_DECODERS], len=0, i, max_layer, au_len, stop_dec;
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;
    for(i =0; i < MAX_DECODERS; i++)  {
    	//fixme: au_len is unused
    	if(i==0)
    		stop_dec = stop_dec1;
    	if(i==1)
    		stop_dec = stop_dec2;
    	au_len = !stop_dec ? packet[i].size : 0;
        got_picture[i]                 = 0;
        openHevcContext                = openHevcContexts->wraper[i];
        openHevcContext->c->quality_id = openHevcContexts->active_layer;
//        printf("quality_id %d \n", openHevcContext->c->quality_id);
        if (i <= openHevcContexts->active_layer) { // pour la auite remplacer par l = 1
            openHevcContext->avpkt.size = au_len;
            openHevcContext->avpkt.data = (uint8_t *) packet[i].data;
        } else {
            openHevcContext->avpkt.size = 0;
            openHevcContext->avpkt.data = NULL;
        }
        openHevcContext->avpkt.pts  = packet[i].pts;
        len                         = avcodec_decode_video2(openHevcContext->c, openHevcContext->picture,
                                                             &got_picture[i], &openHevcContext->avpkt);

        if(i+1 < openHevcContexts->nb_decoders)

        	//Fixme: This way of passing base layer frame reference to each other is bad and should be corrected
        	//We don't know what the first decoder could be doing with its BL_frame (modifying or deleting it)
        	//A cleanest way to do things would be to handle the h264 decoder from the first decoder, but the main issue
        	//would be finding a way to keep giving AVPacket, to h264 when required until the BL_frames required by HEVC
        	//are decoded and available.
           openHevcContexts->wraper[i+1]->c->BL_frame = openHevcContexts->wraper[i]->c->BL_frame;
    }
    if (len < 0) {
        fprintf(stderr, "Error while decoding frame \n");
        return -1;
    }
    if(openHevcContexts->set_display)
            max_layer = openHevcContexts->display_layer;
        else
            max_layer = openHevcContexts->active_layer;

        for(i=max_layer; i>=0; i--) {
            if(got_picture[i]){
                if(i == openHevcContexts->display_layer) {
                    if (i >= 0 && i < openHevcContexts->nb_decoders)
                        openHevcContexts->display_layer = i;
                    return got_picture[i];
                }
             //   fprintf(stderr, "Display layer %d  \n", i);

            }

        }
    return 0;
}



void libOpenHevcCopyExtraData(OpenHevc_Handle openHevcHandle, unsigned char *extra_data, int extra_size_alloc)
{
    int i;
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;
    for(i =0; i <= openHevcContexts->active_layer; i++)  {
        openHevcContext = openHevcContexts->wraper[i];
        openHevcContext->c->extradata = (uint8_t*)av_mallocz(extra_size_alloc);
        memcpy( openHevcContext->c->extradata, extra_data, extra_size_alloc);
        openHevcContext->c->extradata_size = extra_size_alloc;
	}
}


void libOpenHevcGetPictureInfo(OpenHevc_Handle openHevcHandle, OpenHevc_FrameInfo *openHevcFrameInfo)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext  = openHevcContexts->wraper[openHevcContexts->display_layer];
    AVFrame                 *picture          = openHevcContext->picture;

    openHevcFrameInfo->nYPitch    = picture->linesize[0];

    switch (picture->format) {
        case PIX_FMT_YUV420P   :
        case PIX_FMT_YUV420P9  :
        case PIX_FMT_YUV420P10 :
        case PIX_FMT_YUV420P12 :
            openHevcFrameInfo->nUPitch    = picture->linesize[1];
            openHevcFrameInfo->nVPitch    = picture->linesize[2];
            openHevcFrameInfo->color_format = YUV420;
            break;
        case PIX_FMT_YUV422P   :
        case PIX_FMT_YUV422P9  :
        case PIX_FMT_YUV422P10 :
        case PIX_FMT_YUV422P12 :
            openHevcFrameInfo->nUPitch    = picture->linesize[1];
            openHevcFrameInfo->nVPitch    = picture->linesize[2];
            openHevcFrameInfo->color_format = YUV422;
            break;
        case PIX_FMT_YUV444P   :
        case PIX_FMT_YUV444P9  :
        case PIX_FMT_YUV444P10 :
        case PIX_FMT_YUV444P12 :
            openHevcFrameInfo->nUPitch    = picture->linesize[1];
            openHevcFrameInfo->nVPitch    = picture->linesize[2];
            openHevcFrameInfo->color_format = YUV444;
            break;
        default :
            openHevcFrameInfo->nUPitch    = picture->linesize[1];
            openHevcFrameInfo->nVPitch    = picture->linesize[2];
            break;
    }

    switch (picture->format) {
        case PIX_FMT_YUV420P   :
        case PIX_FMT_YUV422P   :
        case PIX_FMT_YUV444P   :
            openHevcFrameInfo->nBitDepth  =  8;
            break;
        case PIX_FMT_YUV420P9  :
        case PIX_FMT_YUV422P9  :
        case PIX_FMT_YUV444P9  :
            openHevcFrameInfo->nBitDepth  =  9;
            break;
        case PIX_FMT_YUV420P10 :
        case PIX_FMT_YUV422P10 :
        case PIX_FMT_YUV444P10 :
            openHevcFrameInfo->nBitDepth  = 10;
            break;
        case PIX_FMT_YUV420P12 :
        case PIX_FMT_YUV422P12 :
        case PIX_FMT_YUV444P12 :
            openHevcFrameInfo->nBitDepth  = 12;
            break;
        default               : openHevcFrameInfo->nBitDepth   =  8; break;
    }

    openHevcFrameInfo->nWidth                  = picture->width;
    openHevcFrameInfo->nHeight                 = picture->height;
    openHevcFrameInfo->sample_aspect_ratio.num = picture->sample_aspect_ratio.num;
    openHevcFrameInfo->sample_aspect_ratio.den = picture->sample_aspect_ratio.den;
    openHevcFrameInfo->frameRate.num           = openHevcContext->c->time_base.den;
    openHevcFrameInfo->frameRate.den           = openHevcContext->c->time_base.num;
    openHevcFrameInfo->display_picture_number  = picture->display_picture_number;
    openHevcFrameInfo->flag                    = (picture->top_field_first << 2) | picture->interlaced_frame; //progressive, interlaced, interlaced bottom field first, interlaced top field first.
    openHevcFrameInfo->nTimeStamp              = picture->pkt_pts;
}

void libOpenHevcGetPictureInfoCpy(OpenHevc_Handle openHevcHandle, OpenHevc_FrameInfo *openHevcFrameInfo)
{

    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext  = openHevcContexts->wraper[openHevcContexts->display_layer];
    AVFrame                 *picture          = openHevcContext->picture;

    switch (picture->format) {
        case PIX_FMT_YUV420P   :
            openHevcFrameInfo->color_format = YUV420;
            openHevcFrameInfo->nYPitch    = picture->width;
            openHevcFrameInfo->nUPitch    = picture->width >> 1;
            openHevcFrameInfo->nVPitch    = picture->width >> 1;
            break;
        case PIX_FMT_YUV420P9  :
        case PIX_FMT_YUV420P10 :
        case PIX_FMT_YUV420P12 :
            openHevcFrameInfo->color_format = YUV420;
            openHevcFrameInfo->nYPitch    = picture->width << 1;
            openHevcFrameInfo->nUPitch    = picture->width;
            openHevcFrameInfo->nVPitch    = picture->width;
            break;
        case PIX_FMT_YUV422P   :
            openHevcFrameInfo->color_format = YUV422;
            openHevcFrameInfo->nYPitch    = picture->width;
            openHevcFrameInfo->nUPitch    = picture->width >> 1;
            openHevcFrameInfo->nVPitch    = picture->width >> 1;
            break;
        case PIX_FMT_YUV422P9  :
        case PIX_FMT_YUV422P10 :
        case PIX_FMT_YUV422P12 :
            openHevcFrameInfo->color_format = YUV422;
            openHevcFrameInfo->nYPitch    = picture->width << 1;
            openHevcFrameInfo->nUPitch    = picture->width;
            openHevcFrameInfo->nVPitch    = picture->width;
            break;
        case PIX_FMT_YUV444P   :
            openHevcFrameInfo->color_format = YUV444;
            openHevcFrameInfo->nYPitch    = picture->width;
            openHevcFrameInfo->nUPitch    = picture->width;
            openHevcFrameInfo->nVPitch    = picture->width;
            break;
        case PIX_FMT_YUV444P9  :
        case PIX_FMT_YUV444P10 :
        case PIX_FMT_YUV444P12 :
            openHevcFrameInfo->color_format = YUV444;
            openHevcFrameInfo->nYPitch    = picture->width << 1;
            openHevcFrameInfo->nUPitch    = picture->width << 1;
            openHevcFrameInfo->nVPitch    = picture->width << 1;
            break;
        default :
            openHevcFrameInfo->color_format = YUV420;
            openHevcFrameInfo->nYPitch    = picture->width;
            openHevcFrameInfo->nUPitch    = picture->width >> 1;
            openHevcFrameInfo->nVPitch    = picture->width >> 1;
            break;
    }

    switch (picture->format) {
        case PIX_FMT_YUV420P   :
        case PIX_FMT_YUV422P   :
        case PIX_FMT_YUV444P   :
            openHevcFrameInfo->nBitDepth  =  8;
            break;
        case PIX_FMT_YUV420P9  :
        case PIX_FMT_YUV422P9  :
        case PIX_FMT_YUV444P9  :
            openHevcFrameInfo->nBitDepth  =  9;
            break;
        case PIX_FMT_YUV420P10 :
        case PIX_FMT_YUV422P10 :
        case PIX_FMT_YUV444P10 :
            openHevcFrameInfo->nBitDepth  = 10;
            break;
        case PIX_FMT_YUV420P12 :
        case PIX_FMT_YUV422P12 :
        case PIX_FMT_YUV444P12 :
            openHevcFrameInfo->nBitDepth  = 12;
            break;
        default               : openHevcFrameInfo->nBitDepth   =  8; break;
    }

    openHevcFrameInfo->nWidth                  = picture->width;
    openHevcFrameInfo->nHeight                 = picture->height;
    openHevcFrameInfo->sample_aspect_ratio.num = picture->sample_aspect_ratio.num;
    openHevcFrameInfo->sample_aspect_ratio.den = picture->sample_aspect_ratio.den;
    openHevcFrameInfo->frameRate.num           = openHevcContext->c->time_base.den;
    openHevcFrameInfo->frameRate.den           = openHevcContext->c->time_base.num;
    openHevcFrameInfo->display_picture_number  = picture->display_picture_number;
    openHevcFrameInfo->flag                    = (picture->top_field_first << 2) | picture->interlaced_frame; //progressive, interlaced, interlaced bottom field first, interlaced top field first.
    openHevcFrameInfo->nTimeStamp              = picture->pkt_pts;
}

int libOpenHevcGetOutput(OpenHevc_Handle openHevcHandle, int got_picture, OpenHevc_Frame *openHevcFrame)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext  = openHevcContexts->wraper[openHevcContexts->display_layer];

    if (got_picture) {
        openHevcFrame->pvY       = (void *) openHevcContext->picture->data[0];
        openHevcFrame->pvU       = (void *) openHevcContext->picture->data[1];
        openHevcFrame->pvV       = (void *) openHevcContext->picture->data[2];

        libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrame->frameInfo);
    }
    return 1;
}

int libOpenHevcGetOutputCpy(OpenHevc_Handle openHevcHandle, int got_picture, OpenHevc_Frame_cpy *openHevcFrame)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext  = openHevcContexts->wraper[openHevcContexts->display_layer];

    int y;
    int y_offset, y_offset2;
    if( got_picture ) {
        unsigned char *Y = (unsigned char *) openHevcFrame->pvY;
        unsigned char *U = (unsigned char *) openHevcFrame->pvU;
        unsigned char *V = (unsigned char *) openHevcFrame->pvV;
        int height, format;
        int src_stride;
        int dst_stride;
        int src_stride_c;
        int dst_stride_c;

        libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrame->frameInfo);
        format = openHevcFrame->frameInfo.color_format == YUV420 ? 1 : 0;
        src_stride = openHevcFrame->frameInfo.nYPitch;
        src_stride_c = openHevcFrame->frameInfo.nUPitch;
        height = openHevcFrame->frameInfo.nHeight;

        libOpenHevcGetPictureInfoCpy(openHevcHandle, &openHevcFrame->frameInfo);
        dst_stride = openHevcFrame->frameInfo.nYPitch;
        dst_stride_c = openHevcFrame->frameInfo.nUPitch;

        y_offset = y_offset2 = 0;

        for (y = 0; y < height; y++) {
            memcpy(&Y[y_offset2], &openHevcContext->picture->data[0][y_offset], dst_stride);
            y_offset  += src_stride;
            y_offset2 += dst_stride;
        }

        y_offset = y_offset2 = 0;

        for (y = 0; y < height >> format; y++) {
            memcpy(&U[y_offset2], &openHevcContext->picture->data[1][y_offset], dst_stride_c);
            memcpy(&V[y_offset2], &openHevcContext->picture->data[2][y_offset], dst_stride_c);
            y_offset  += src_stride_c;
            y_offset2 += dst_stride_c;
        }
    }
    return 1;
}

void libOpenHevcSetDebugMode(OpenHevc_Handle openHevcHandle, int val)
{
    if (val == 1)
        av_log_set_level(AV_LOG_DEBUG);
}

void libOpenHevcSetThreadAffinity(OpenHevc_Handle openHevcHandle, uint32_t val){
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;
    int i;

    for (i = 0; i < openHevcContexts->nb_decoders; i++) {
        openHevcContext = openHevcContexts->wraper[i];

        av_opt_set_int(openHevcContext->c->priv_data, "thread-affinity", val, 0);
    }
}

void libOpenHevcSetActiveDecoders(OpenHevc_Handle openHevcHandle, int val)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    if (val >= 0 && val < openHevcContexts->nb_decoders)
        openHevcContexts->active_layer = val;
    else {
        fprintf(stderr, "The requested layer %d can not be decoded (it exceeds the number of allocated decoders %d ) \n", val, openHevcContexts->nb_decoders);
        openHevcContexts->active_layer = openHevcContexts->nb_decoders-1;
    }
}

void libOpenHevcSetViewLayers(OpenHevc_Handle openHevcHandle, int val)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    //openHevcContexts->set_display = 1;
    if (val >= 0 && val < openHevcContexts->nb_decoders)
        openHevcContexts->display_layer = val;
    else {
        fprintf(stderr, "The requested layer %d can not be viewed (it exceeds the number of allocated decoders %d ) \n", val, openHevcContexts->nb_decoders);
        openHevcContexts->display_layer = openHevcContexts->nb_decoders-1;
    }
}


void libOpenHevcSetCheckMD5(OpenHevc_Handle openHevcHandle, int val)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;
    int i;

    for (i = 0; i < openHevcContexts->nb_decoders; i++) {
        openHevcContext = openHevcContexts->wraper[i];

        av_opt_set_int(openHevcContext->c->priv_data, "decode-checksum", val, 0);
    }
}

void libOpenHevcSetTemporalLayer_id(OpenHevc_Handle openHevcHandle, int val)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;
    int i;

    for (i = 0; i < openHevcContexts->nb_decoders; i++) {
        openHevcContext = openHevcContexts->wraper[i];
        av_opt_set_int(openHevcContext->c->priv_data, "temporal-layer-id", val, 0);
    }
    
}

void libOpenHevcSetNoCropping(OpenHevc_Handle openHevcHandle, int val)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;
    int i;

    for (i = 0; i < openHevcContexts->nb_decoders; i++) {
        openHevcContext = openHevcContexts->wraper[i];
        av_opt_set_int(openHevcContext->c->priv_data, "no-cropping", val, 0);
    }
}
/** Green arguments parse and parameters init */
void libOpenHevcInitGreen(OpenHevc_Handle openHevcHandle, char *green_param, int green_verbose)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;

    int alevel=0;
    int green_luma=7;
    int green_chroma=4;
    int green_dbf_on=1;
    int green_sao_on=1;

    int len = strlen(green_param);
    char buffer[3]="0";
    int i = (len == 5) ? 0 : 1;

    strncpy(buffer,green_param,(1+i)*sizeof(char));
    alevel = atoi(buffer);

    strcpy(buffer,"0");

    if( alevel != 0){
        strncpy(buffer,green_param+1+i,sizeof(char));
        green_luma = atoi(buffer);

        strncpy(buffer,green_param+2+i,sizeof(char));
        green_chroma = atoi(buffer);

        strncpy(buffer,green_param+3+i,sizeof(char));
        green_sao_on = atoi(buffer);

        strncpy(buffer,green_param+4+i,sizeof(char));
        green_dbf_on = atoi(buffer);

    }


    for (i = 0; i < openHevcContexts->nb_decoders; i++) {
        openHevcContext = openHevcContexts->wraper[i];
        av_opt_set_int(openHevcContext->c->priv_data, "green-a-level", alevel, 0);
        av_opt_set_int(openHevcContext->c->priv_data, "green-luma", green_luma, 0);
        av_opt_set_int(openHevcContext->c->priv_data, "green-chroma", green_chroma, 0);
        av_opt_set_int(openHevcContext->c->priv_data, "green-dbf-on", green_dbf_on, 0);
        av_opt_set_int(openHevcContext->c->priv_data, "green-sao-on", green_sao_on, 0);
        av_opt_set_int(openHevcContext->c->priv_data, "green-verbose", green_verbose, 0);
    }
}


void libOpenHevcClose(OpenHevc_Handle openHevcHandle)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext;
    int i;

    for (i = openHevcContexts->nb_decoders-1; i >=0 ; i--){
        openHevcContext = openHevcContexts->wraper[i];
        avcodec_close(openHevcContext->c);
        av_parser_close(openHevcContext->parser);
        av_freep(&openHevcContext->c);
        av_freep(&openHevcContext->picture);
        av_freep(&openHevcContext);
    }
    av_freep(&openHevcContexts->wraper);
    av_freep(&openHevcContexts);
}

void libOpenHevcFlush(OpenHevc_Handle openHevcHandle)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext  = openHevcContexts->wraper[openHevcContexts->active_layer];

    openHevcContext->codec->flush(openHevcContext->c);
}

void libOpenHevcFlushSVC(OpenHevc_Handle openHevcHandle, int decoderId)
{
    OpenHevcWrapperContexts *openHevcContexts = (OpenHevcWrapperContexts *) openHevcHandle;
    OpenHevcWrapperContext  *openHevcContext  = openHevcContexts->wraper[decoderId];

    openHevcContext->codec->flush(openHevcContext->c);
}

const char *libOpenHevcVersion(OpenHevc_Handle openHevcHandle)
{
    return "OpenHEVC v"NV_VERSION;
}


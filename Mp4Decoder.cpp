

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
 
using namespace std;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
 
  
#define MP4toRGB 1
#define WRITE_RGB 0

#define MP4toYUV 1
#define WRITE_YUV 0

typedef void (*decodeFrameCallback)( unsigned char *pRGBData , unsigned char *pYUVData, unsigned int nWidth, unsigned int nHeight );


void SaveFramePPM(uint8_t *pData_rgb, int width, int height, int iFrame)
{
	if(iFrame>10)
		return;

    FILE *pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "%d.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    fwrite(pData_rgb, 1, height*width*3, pFile);

    // Close file
    fclose(pFile);
}


int   Mp4Decoder( decodeFrameCallback  cb){
 
    av_register_all();
 
    AVFormatContext *pFormatCtx;
 
    pFormatCtx = avformat_alloc_context();
 
    char filepath[] = "1421735998235.mp4";
 
    if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
        printf("Can't open the file\n");
        return -1;
    }
 
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
 
    av_dump_format(pFormatCtx, 0, filepath, 0);
 
	// --------------------------------------------------------- //
 
    int i, videoIndex = -1; 
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }
 
    if (videoIndex == -1)
        return -1;
    // --------------------------------------------------------- //
 
    AVCodecContext *pCodecCtx;
 
    AVCodec *pCodec;
 
    pCodecCtx = pFormatCtx->streams[videoIndex]->codec;
 
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        printf("Unsupported codec!\n");
        return -1;
    }
 
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }
 
    AVFrame *pFrame, *pFrameYUV;
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
 
    uint8_t *out_buffer;
    int numBytes;
    numBytes = avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width,
        pCodecCtx->height);
 
    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
 
    avpicture_fill((AVPicture*) pFrameYUV, out_buffer, PIX_FMT_YUV420P,
        pCodecCtx->width, pCodecCtx->height);

 
	//*************************************************************//
 
    int frameFinished;
    AVPacket packet;
    av_new_packet(&packet, numBytes);
    int ret;
 
    static struct SwsContext *img_convert_ctx;

//------------------------------------------------------//
#if (MP4toRGB==0)
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
        pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
        PIX_FMT_YUV420P,
        SWS_BICUBIC, NULL, NULL, NULL);
//--------------------------------------//
#else				
	img_convert_ctx = sws_getContext( pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
                          pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_POINT,
						 NULL, NULL, NULL);
	//-------------------------------------
	uint8_t *out_buffer_rgb;
    int numBytes_rgb;
    numBytes_rgb = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
        pCodecCtx->height); 
    out_buffer_rgb = (uint8_t *) av_malloc(numBytes_rgb * sizeof(uint8_t));
   
	printf("numBytes_rgb %d\n",numBytes_rgb);
	//-------------------------------------
	uint8_t* outData_rgb[4];
	int outLinesize_rgb[4];
    //準備output的buffer pointer
     outData_rgb[0] = out_buffer_rgb;
     outData_rgb[1] =
     outData_rgb[2] =
     outData_rgb[3] = NULL;
     outLinesize_rgb[0] = pCodecCtx->width * 3;
     outLinesize_rgb[1] = outLinesize_rgb[2] = outLinesize_rgb[3] = 0;
#endif
//------------------------------------------------------//

//store yuv
//------------------------------------------------------// 
#if (MP4toYUV==1)
	#if (WRITE_YUV==1) 
	    FILE *pFile=fopen("output.yuv", "wb");
	    if(pFile == NULL)
		    return -1;
    #endif
    //------------------------------------
		 uint8_t *out_buffer_yuv = (uint8_t*)_mm_malloc(pCodecCtx->width*pCodecCtx->height*2 ,16);
     //-------------------------------------
#endif
//------------------------------------------------------//
 
    while (av_read_frame(pFormatCtx, &packet) >= 0) { 
        if (packet.stream_index == videoIndex) {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet); 
            if (ret >= 0) { 
                if (frameFinished) {					
//store yuv
//------------------------------------------------------//				 
#if  (MP4toYUV==1)
        int i = 0; 
        unsigned char *pFrameTemp;   unsigned char *pYuvTemp=out_buffer_yuv; 
             
		pFrameTemp = (unsigned char*)pFrame->data[0];
        for (i = 0 ; i < pCodecCtx->height; i++){  
 #if (WRITE_YUV==1)
            fwrite(pFrameTemp + i * pFrame->linesize[0], 1,  pCodecCtx->width, pFile);
 #endif
             memcpy(pYuvTemp, pFrameTemp + i * pFrame->linesize[0],   pCodecCtx->width);
             pYuvTemp +=  pCodecCtx->width;
        }
		pFrameTemp = (unsigned char*)pFrame->data[1];
	    for (i = 0 ; i < pCodecCtx->height/2; i++){      
 #if (WRITE_YUV==1)
			fwrite(pFrameTemp + i * pFrame->linesize[1], 1,  pCodecCtx->width/2, pFile);
 #endif 
            memcpy(pYuvTemp, pFrameTemp + i * pFrame->linesize[1], pCodecCtx->width/2);
            pYuvTemp +=  pCodecCtx->width/2;
		}

        pFrameTemp = (unsigned char*)pFrame->data[2];
		for (i = 0 ; i < pCodecCtx->height/2; i++){
 #if (WRITE_YUV==1)
			fwrite(pFrameTemp + i * pFrame->linesize[2], 1,  pCodecCtx->width/2, pFile);
 #endif
            memcpy(pYuvTemp, pFrameTemp + i * pFrame->linesize[2], pCodecCtx->width/2);
            pYuvTemp +=  pCodecCtx->width/2;
 		}
 
#endif
//sws_scale RGB
//------------------------------------------------------//
#if (MP4toRGB==1)
		sws_scale(img_convert_ctx, (const uint8_t* const *) pFrame->data,  pFrame->linesize , 0,
			pCodecCtx->height, outData_rgb, outLinesize_rgb);
#endif
//------------------------------------------------------//
		cb(out_buffer_rgb,out_buffer_yuv, pCodecCtx->width ,pCodecCtx->height);

		 
                 }
            }
        }
        av_free_packet(&packet);
 
    }// while (av_read_frame(pFormatCtx, &packet) >= 0)

//*************************************************************//
#if (MP4toRGB==1)
    av_free(out_buffer_rgb);	
#endif
#if (MP4toYUV==1)
    _mm_free(out_buffer_yuv);
	#if (WRITE_YUV==1) 
        fclose(pFile);
    #endif
#endif
    av_frame_free(&pFrame);
    av_frame_free(&pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
 
    return 0;
}
//===========================================================
void decodeFrame( unsigned char *pRGBData , unsigned char *pYUVData, unsigned int nWidth, unsigned int nHeight ){

#if (WRITE_RGB==1)
    static int iFrame=0;
		SaveFramePPM(pRGBData, nWidth ,nHeight, iFrame);
		iFrame++;
#endif
}
 
int _tmain(int argc, _TCHAR* argv[])
{

    Mp4Decoder( decodeFrame );
    getchar();
	return 0;
 
}
 
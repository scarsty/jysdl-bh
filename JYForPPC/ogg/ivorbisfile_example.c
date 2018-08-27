///********************************************************************
// *                                                                  *
// * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
// *                                                                  *
// * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
// * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
// * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
// *                                                                  *
// * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
// * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
// *                                                                  *
// ********************************************************************
//
// function: simple example decoder using vorbisidec
//
// ********************************************************************/
//
///* Takes a vorbis bitstream from stdin and writes raw stereo PCM to
//   stdout using vorbisfile. Using vorbisfile is much simpler than
//   dealing with libvorbis. */
//
//#include <stdio.h>
//#include <stdlib.h>
////#include <vorbis/ivorbiscodec.h>
////#include <vorbis/ivorbisfile.h>
//#include "ivorbiscodec.h"
//#include "ivorbisfile.h"
//
//#ifdef _WIN32 /* We need the following two to set stdin/stdout to binary */
//#include <io.h>
//#include <fcntl.h>
//#endif
//
//FILE *STTIN;
//FILE *STTOUT;
//
//char pcmout[4096]; /* take 4k out of the data segment, not the stack */
//
//int main(){
//  OggVorbis_File vf;
//  int eof=0;
//  int current_section;
//  long csk=0,tm=0;
//
//#ifdef _WIN32 /* We need to set stdin/stdout to binary mode. Damn windows. */
//  /* Beware the evil ifdef. We avoid these where we can, but this one we 
//     cannot. Don't add any more, you'll probably go to hell if you do. */
////  _setmode( _fileno( stdin ), _O_BINARY );
////  _setmode( _fileno( stdout ), _O_BINARY );
//  if ((STTIN = fopen("we56.ogg", "rb")) == NULL){
//	  printf("Could not find Mists_of_Time-4T.ogg .\n");
//  }
//  STTOUT = fopen("Tremor_output.pcm", "wb");
//#endif
//
////  if(ov_open(stdin, &vf, NULL, 0) < 0) {
//  if(ov_open(STTIN, &vf, NULL, 0) < 0) {
//      fprintf(stderr,"Input does not appear to be an Ogg bitstream.\n");
//      exit(1);
//  }
//
//  /* Throw the comments plus a few lines about the bitstream we're
//     decoding */
//  {
//    char **ptr=ov_comment(&vf,-1)->user_comments;
//    vorbis_info *vi=ov_info(&vf,-1);
//    while(*ptr){
//      fprintf(stderr,"%s\n",*ptr);
//      ++ptr;
//    }
//    fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi->channels,vi->rate);
//    fprintf(stderr,"\nDecoded length: %ld samples\n",
//	    (long)ov_pcm_total(&vf,-1));
//    fprintf(stderr,"Encoded by: %s\n\n",ov_comment(&vf,-1)->vendor);
//  }
//  
//  csk = ov_seekable(&vf);
//  ov_pcm_seek(&vf, 32*34);
//  tm = ov_time_tell(&vf);
//  while(!eof){
//    long ret=ov_read(&vf,pcmout,sizeof(pcmout),&current_section);
//    if (ret == 0) {
//      /* EOF */
//      eof=1;
//    } else if (ret < 0) {
//      /* error in the stream.  Not a problem, just reporting it in
//	 case we (the app) cares.  In this case, we don't. */
//    } else {
//      /* we don't bother dealing with sample rate changes, etc, but
//	 you'll have to*/
////      fwrite(pcmout,1,ret,stdout);
//	  fwrite(pcmout,1,ret,STTOUT);
//    }
//  }
//
//  /* cleanup */
//  ov_clear(&vf);
//    
//  fprintf(stderr,"Done.\n");
//  return(0);
//}

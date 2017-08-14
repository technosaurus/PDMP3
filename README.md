PDMP3
=====

Public domain mp3 decoder

This project is a fork of the mp3 decoder written by Krister Lagerstrom as part of a master's thesis. This version implements a subset op the libmpg123 API which should be a 100% compatible replacement. If it's not that is considered a bug. In theory more libmpg123 compatible functions could be implemented quite easily.

Available functions for the streaming API are:
`
pdmp3_handle *pdmp3_new(const char *decoder,int *error);
void pdmp3_delete(pdmp3_handle *id);
int pdmp3_open_feed(pdmp3_handle *id);
int pdmp3_feed(pdmp3_handle *id,const unsigned char *in,size_t size);
int pdmp3_read(pdmp3_handle *id,unsigned char *outmemory,size_t outsize,size_t *done);
int pdmp3_decode(pdmp3_handle *id,const unsigned char *in,size_t insize,unsigned char *out,size_t outsize,size_t *done);
int pdmp3_getformat(pdmp3_handle *id,long *rate,int *channels,int *encoding);
int pdmp3_info(pdmp3_handle *id,struct pdpm3_frameinfo *info);
`

The following code allows approximating the duration of the current stream if
the length of the stream is known:

`
struct pdpm3_frameinfo info;
int enc, channels;
long rate;

if (pdmp3_getformat(handle->id, &rate, &channels, &enc) == MPG123_OK) {
  struct pdpm3_frameinfo info;
  if (pdmp3_info(handle->id,&info) == MPG123_OK) {
    double no_samples = (double)rate/(info.bitrate/8.0)*file_size_bytes;
  }
}
`

TODO
----
  * cleanup,
  * switch to bitfields,
  * fix some of the horribly nested logic

>    License: My work is placed in the public domain. You may do whatever you wish with it, including using it for commercial applications.

The thesis and original code are available at:
 * https://sites.google.com/a/kmlager.com/www/projects

From the abstract:

>    Digital compression of audio data is important due to the bandwidth and storage limitations inherent in networks and computers. Algorithms based on perceptual coding are effective and have become feasible with faster computers. The ISO standard 11172-3 MPEG-1 layer III (a.k.a. MP3) is a perceptual codec that is presently very common for compression of CD quality music. An MP3 decoder has a complex structure and is computationally demanding.

>    The purpose of this master's thesis is to present a tutorial on the standard. We have analysed several algorithms suitable for implementing an MP3 decoder, their advantages and disadvantages with respect to speed, memory demands and implementation complexity. We have also designed and implemented a portable reference MP3 decoder in C.

If you are trying to implement an mp3 decoder and found the 11172-3 reference to be vague, the thesis is a a good read.

Some other good references are:
 * http://blog.bjrn.se/2008/10/lets-build-mp3-decoder.html
 * http://www.multiweb.cz/twoinches/mp3inside.htm
 * http://www.mp3-converter.com/mp3codec/
 * http://oreilly.com/catalog/9781565926615

#ifndef __ENCODE_H
#define __ENCODE_H

int sqlite_decode_binary(const unsigned char *in, int in_bufsize, unsigned char *out, int out_bufsize);
int sqlite_encode_binary(const unsigned char *in, int n, unsigned char *out);


#endif

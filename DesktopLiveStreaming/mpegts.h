


#ifndef _MPEGTS_H_INCLUDED_
#define _MPEGTS_H_INCLUDED_
#include <stdint.h>
#include <stdio.h>

typedef unsigned int        mpegts_uint_t;
typedef int        mpegts_int_t;

typedef unsigned char   u_char;

#define MPEGTS_OK (1)
#define MPEGTS_ERROR (2)

typedef struct {
	FILE    *fd;
    int    size;
    //u_char      buf[16];
    //u_char      iv[16];
} mpegts_file_t;

typedef struct {
    uint64_t    pts;
    uint64_t    dts;
    mpegts_uint_t  pid;
    mpegts_uint_t  sid;
    mpegts_uint_t  cc;
    unsigned    key:1;
} mpegts_frame_t;

typedef struct mpegts_buf_s  mpegts_buf_t;

struct mpegts_buf_s {
	u_char          *pos;
	u_char          *last;
	//off_t            file_pos;
	//off_t            file_last;

	u_char          *start;         /* start of buffer */
	u_char          *end;           /* end of buffer */
	
};

mpegts_int_t mpegts_open_file(mpegts_file_t *file, char *path);
mpegts_int_t mpegts_close_file(mpegts_file_t *file);
mpegts_int_t mpegts_write_frame(mpegts_file_t *file,
    mpegts_frame_t *f, mpegts_buf_t *b);

#endif 

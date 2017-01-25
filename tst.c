#define _XOPEN_SOURCE_EXTENDED
#define _LARGEFILE64_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <quicktime.h>
#include <libdv/dv.h>

#include <curses.h>
#include <term.h>

#include "eia608.h"
#include "smpte.h"

#define DV_PAL_SIZE (12 * 150 * 80)
#define DV_NTSC_SIZE  (10 * 150 * 80)

char* cls = NULL;

void display(const char* header, eia608_t* cc) {
    int i,j;
    cchar_t wch;

    move(0,0);
    printw("%s\n", header);

    wchar_t** scr = eia608_get_screen(cc);
    int** att = eia608_get_attributes(cc);
    for(i = 0; i < EIA608_ROWS; ++i) {
	for(j = 0; j < EIA608_COLUMNS; ++j) {
	    wchar_t ch = scr[i][j];
	    int a = att[i][j];
	    wch.chars[0] = ch == 0 ? L' ' : ch;
	    wch.chars[1] = 0;
	    wch.attr = COLOR_PAIR((a & 0x7) + 1);
	    if (a & EIA608_UNDERLINE)
		wch.attr |= A_UNDERLINE;
	    if (a & EIA608_ITALIC)
		wch.attr |= A_STANDOUT; /* curses doesn't have italics */
	    add_wch(&wch);;
	}
	printw("\n");
    }

    refresh();
}

int main(int argc, char** argv) {
    quicktime_t* qtfile;
    char* codec;
    int framesize;
    unsigned char* buffer;
    dv_decoder_t* dv;
    uint8_t cc[4];
    long i, nframes;
    eia608_t* decoder;
    smpte_t* tc;
    char tcbuf[SMPTE_STR_LEN];
    int uselibqt;
    int fd = 0;

    struct timeval tv, now;
    struct timespec delay;

    setlocale(LC_ALL, "");

    if (argc != 2) {
	fprintf(stderr, "please provide one arg, the name of the file you want.\n");
	return 1;
    }

    qtfile = quicktime_open(argv[1], 1, 0);

    if (qtfile) {
	uselibqt = 1;
	if (quicktime_video_tracks(qtfile) != 1) {
	    fprintf(stderr, "expected file with exactly 1 video track.\n");
	    return 1;
	}

	codec = quicktime_video_compressor(qtfile, 0);
	if (strcmp(codec, QUICKTIME_DV) &&
	    strcmp(codec, QUICKTIME_DV_AVID) &&
	    strcmp(codec, QUICKTIME_DV_AVID_A)) {
	    fprintf(stderr, "expected DV video codec, got %s.\n", codec);
	    return 1;
	}

	framesize = quicktime_frame_size(qtfile, 0, 0);
	if (framesize == 0) {
	    fprintf(stderr, "couldn't get frame size.\n");
	    return 1;
	}
	nframes = quicktime_video_length(qtfile, 0);
    } else {
	off64_t off;

	uselibqt = 0;
	fd = open(argv[1], O_RDONLY);
	if (!fd) {
	    perror("open");
	    return 1;
	}

	buffer = malloc(4);
	read(fd, buffer, 4);

	if (!(((buffer[0] & 0xf0) == 0x10) && ((buffer[1] & 0xf0) == 0x00) && (buffer[2] == 0x00))) {
	    close(fd);
	    fprintf(stderr, "file does not appear to be either quicktime or raw DV\n");
	    return 1;
	}

	framesize = DV_NTSC_SIZE;
	free(buffer);
	off = lseek64(fd, 0, SEEK_END); /* go to end of file */
	nframes = off / framesize;
	lseek(fd, 0, SEEK_SET); /* back to beginning */
    }
    printf("%i\n", framesize);
    assert(framesize == DV_PAL_SIZE || framesize == DV_NTSC_SIZE);
    buffer = malloc(framesize);

    if (!buffer) {
	perror("couldn't malloc");
	return 1;
    }

    dv = dv_decoder_new(0, 0, 0);
    if(!dv) {
	fprintf(stderr, "couldn't create DV decoder\n");
    }

    decoder = eia608_new();
    tc = (framesize == DV_NTSC_SIZE ? smpte_new(1, 30) : smpte_new(0, 25));

    initscr();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    init_pair(6, COLOR_YELLOW, COLOR_BLACK);
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);

    gettimeofday(&tv, NULL);

    for(i = 0; i < nframes; ++i) {
	if (uselibqt) {
	    assert(quicktime_frame_size(qtfile, i, 0) == framesize);
	    quicktime_read_frame(qtfile, buffer, 0);
	} else {
	    read(fd, buffer, framesize);
	}
	dv_parse_header(dv, buffer);
	dv_parse_packs(dv, buffer);

	if(dv_get_vaux_pack(dv, 0x65, cc) == 0) {
	    eia608_input(decoder, cc);
	    smpte_format(tc, tcbuf);
	    display(tcbuf, decoder);
	}
	tv.tv_usec += (framesize == DV_NTSC_SIZE ? (i % 3 ? 33367 : 33366) : 40000);
	if (tv.tv_usec > 1000000) {
	    tv.tv_usec -= 1000000;
	    tv.tv_sec++;
	}

	gettimeofday(&now, NULL);
	delay.tv_sec = 0;
	delay.tv_nsec = 1000L*((tv.tv_usec-now.tv_usec) + 1000000*(tv.tv_sec-now.tv_sec));

	if (delay.tv_nsec > 0)
	    while(nanosleep(&delay, &delay));


	smpte_incr_frame(tc);
    }

    endwin();
    dv_decoder_free(dv);
    smpte_free(tc);
    eia608_free(decoder);
    free(buffer);
    if (uselibqt)
	quicktime_close(qtfile);

    return 0;
}

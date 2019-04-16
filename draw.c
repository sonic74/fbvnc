#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "draw.h"

#include <linux/mxcfb.h>

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define NLEVELS		(1 << 16)


#define LOGI(...)  printf(__VA_ARGS__)
//#define LOGI(...)
static __u32 marker_val = 1;


static int fd;
static void *fb;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static int bytes_per_pixel;

static int fb_len(void)
{
	return finfo.line_length * vinfo.yres_virtual;
}

static void fb_cmap_save(int save)
{
	static unsigned short red[NLEVELS], green[NLEVELS], blue[NLEVELS];
	struct fb_cmap cmap;

	if (finfo.visual == FB_VISUAL_TRUECOLOR)
		return;

	cmap.start = 0;
	cmap.len = NLEVELS;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;
	ioctl(fd, save ? FBIOGETCMAP : FBIOPUTCMAP, &cmap);
}

void fb_cmap(void)
{
	struct fb_bitfield *color[3] = {
		&vinfo.blue, &vinfo.green, &vinfo.red
	};
	int eye_sensibility[3] = { 2, 0, 1 }; // higher=red, blue, lower=green
	struct fb_cmap cmap;
	unsigned short map[3][NLEVELS];
	int i, j, n, offset;

	if (finfo.visual == FB_VISUAL_TRUECOLOR)
		return;

	for (i = 0, n = vinfo.bits_per_pixel; i < 3; i++) {
		n -= color[eye_sensibility[i]]->length = n / (3 - i);
	}
	n = (1 << vinfo.bits_per_pixel);
	if (n > NLEVELS)
		n = NLEVELS;
	for (i = offset = 0; i < 3; i++) {
		int length = color[i]->length;
		color[i]->offset = offset;
		for (j = 0; j < n; j++) {
			int k = (j >> offset) << (16 - length);
			if (k == (0xFFFF << (16 - length)))
				k = 0xFFFF;
			map[i][j] = k;
		}
		offset += length;
	}

	cmap.start = 0;
	cmap.len = n;
	cmap.red = map[2];
	cmap.green = map[1];
	cmap.blue = map[0];
	cmap.transp = NULL;

	ioctl(fd, FBIOPUTCMAP, &cmap);
}

unsigned fb_mode(void)
{
	return (bytes_per_pixel << 16) | (vinfo.red.length << 8) |
		(vinfo.green.length << 4) | (vinfo.blue.length);
}

static int rotate_=-1;
void setRotate(int rotate) {
	rotate_=rotate;
}

int fb_init(void)
{
	int err = 1;
printf("FBDEV_PATH=%s\n", FBDEV_PATH);
	fd = open(FBDEV_PATH, O_RDWR);
	if (fd == -1)
		goto failed;
	err++;
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
		goto failed;
printf("vinfo.xres=%i  yres=%i  bits_per_pixel=%i  red.offset=%i  green.offset=%i  blue.offset=%i  red.length=%i  green.length=%i  blue.length=%i  rotate=%i\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, vinfo.red.offset, vinfo.green.offset, vinfo.blue.offset, vinfo.red.length, vinfo.green.length, vinfo.blue.length, vinfo.rotate);
	if(rotate_!=-1) {
		vinfo.rotate=rotate_;
		if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) == -1)
			goto failed;
	}
	err++;
	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
		goto failed;
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
	bytes_per_pixel = (vinfo.bits_per_pixel + 7) >> 3;
printf("finfo.line_length/%i=%i\n", bytes_per_pixel, finfo.line_length/bytes_per_pixel);
	fb = mmap(NULL, fb_len(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	err++;
	if (fb == MAP_FAILED)
		goto failed;
	fb_cmap_save(1);
	fb_cmap();
	return 0;
failed:
	perror("fb_init()");
	close(fd);
	return err;
}

/*static*/ rfbBool MallocFrameBuffer(rfbClient* client) {
printf("MallocFrameBuffer()\n\r");
	int i;
	i = fb_init();
	if (i) {
		return FALSE;
	} else {
	client->width = finfo.line_length / bytes_per_pixel;

	client->height = vinfo.yres;

	client->frameBuffer=fb;

	client->format.bitsPerPixel=vinfo.bits_per_pixel;
	client->format.redShift=vinfo.red.offset;
	client->format.greenShift=vinfo.green.offset;
	client->format.blueShift=vinfo.blue.offset;
	client->format.redMax=(1<<vinfo.red.length)-1;
	client->format.greenMax=(1<<vinfo.green.length)-1;
	client->format.blueMax=(1<<vinfo.blue.length)-1;
		return TRUE;
	}
}

void fb_free(void)
{
	fb_cmap_save(0);
	munmap(fb, fb_len());
	close(fd);
}

int fb_rows(void)
{
	return vinfo.yres;
}

int fb_cols(void)
{
	return vinfo.xres;
}

void *fb_mem(int r)
{
	return fb + (r + vinfo.yoffset) * finfo.line_length;
}


void update (__u32 left, __u32 top, __u32 width, __u32 height, __u32 waveform_mode, __u32 update_mode, unsigned int flags, int wait_for_complete) {

	int retval;
	struct mxcfb_update_data upd_data;
    memset(&upd_data, 0, sizeof(struct mxcfb_update_data));

	LOGI("update(%d,%d,%d,%d,%d,%d,%d,%d)  ", left, top, width, height, waveform_mode, update_mode, flags, wait_for_complete);

	upd_data.update_region.left = left;
	upd_data.update_region.top = top;
	if(left+width>vinfo.xres) width=vinfo.xres-left;
	upd_data.update_region.width = width;
	upd_data.update_region.height = height;

    upd_data.waveform_mode = waveform_mode;

    upd_data.update_mode = update_mode;

    upd_data.flags |= flags;

	upd_data.temp = TEMP_USE_AMBIENT;

	if (wait_for_complete) {
		upd_data.update_marker = marker_val++;
	} else {
		upd_data.update_marker = 0;
	}
	LOGI(">>MXCFB_SEND_UPDATE<< offset=%d  ", vinfo.yoffset);
	retval = ioctl(fd, MXCFB_SEND_UPDATE, &upd_data);
	if (retval < 0) {
//		usleep(300000);
//		retval = ioctl(fd, MXCFB_SEND_UPDATE, &upd_data);
        LOGI("retval = 0x%x try again maybe  ", retval);
	}

	if (wait_for_complete) {
		LOGI(">>MXCFB_WAIT_FOR_UPDATE_COMPLETE<<  ");
		retval = ioctl(fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
		if (retval < 0) {
			LOGI("failed. Error=0x%x ", retval);
		}
		LOGI("Done  ");
	}

	LOGI("\r\n");
}

static int x_=INT_MAX, y_=INT_MAX, x2_, y2_;
/*static*/ void GotFrameBufferUpdate(rfbClient* cl, int x, int y, int w, int h) {
	int x2, y2;
	x2=x+w;
	y2=y+h;
	x_=MIN(x_, x);
	y_=MIN(y_, y);
	x2_=MAX(x2_, x2);
	y2_=MAX(y2_, y2);
}

void FinishedFrameBufferUpdate(rfbClient* cl) {
	update(x_, y_, x2_-x_, y2_-y_, WAVEFORM_MODE_A2, UPDATE_MODE_PARTIAL, EPDC_FLAG_FORCE_MONOCHROME, 0);
	x_=vinfo.xres;
	y_=vinfo.yres;
	x2_=0;
	y2_=0;
}

void mxc_epdc_fb_full_refresh(rfbClient* cl) {
	update(0, 0, vinfo.xres, vinfo.yres, WAVEFORM_MODE_GC16, UPDATE_MODE_FULL, 0, 1);
}


void fb_set(int r, int c, void *mem, int len)
{
	memcpy(fb_mem(r) + (c + vinfo.xoffset) * bytes_per_pixel,
		mem, len * bytes_per_pixel);
}

void fill_rgb_conv(int mode, struct rgb_conv *s)
{
	int bits;

	bits = mode & 0xF;  mode >>= 4;
	s->rshl = s->gshl = bits;
	s->bskp = 8 - bits; s->bmax = (1 << bits) -1;
	bits = mode & 0xF;  mode >>= 4;
	s->rshl += bits;
	s->gskp = 8 - bits; s->gmax = (1 << bits) -1;
	bits = mode & 0xF;
	s->rskp = 8 - bits; s->rmax = (1 << bits) -1;
}

unsigned fb_val(int r, int g, int b)
{
	static struct rgb_conv c;

	if (c.rshl == 0)
		fill_rgb_conv(fb_mode(), &c);
	return ((r >> c.rskp) << c.rshl) | ((g >> c.gskp) << c.gshl) 
					 | (b >> c.bskp);
}

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int get_sound(int *compare_buf, int len, int bitspersample)
{
	int sound = 0;
	int max_vol = compare_buf[0];
	int min_vol = compare_buf[0];
	int max_sound = 2;
	int i = 0;
	for(i=0; i<bitspersample-1; i++)
		max_sound *= 2;
	for(i=0; i<len; i++) {
		if(compare_buf[i] > max_vol)
			max_vol = compare_buf[i];
		if(compare_buf[i] < min_vol)
			min_vol = compare_buf[i];
	}
	//printf("minus = %d, max_sound = %d\n", max_vol-min_vol, max_sound);
	if((max_vol-min_vol) > (max_sound/4))
		return 1;
	else
		return 0;
}
int main(int argc, char *argv[])
{
    int fd = 0;
    int ret = 0;
    char id[4] = {0x00};
    int file_size = 0;
    int fmt_size = 0;
    int data_size = 0;
    int num_channels = 0;
    int samplerate = 0;
	int bytesperperiod = 0;
    int bitspersample = 0;
    int bytespersample = 0;
    unsigned char buf[200] = {0x00};
	int *compare_buf = NULL;
	#if 0
    int max_vol_p = 0;
    int max_vol_n = 0;
    int vol = 0;
	#endif
    int i = 0, j = 0;
	int count = 0;
	int sound = 1;
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("open %s failed\n", argv[1]);
        return fd;
    }

    ret = read(fd, id, 4);
    if (ret < 4) {
        printf("read id failed\n");
        return ret;
    }

    printf("id = %s\n", id);

    ret = read(fd, buf, 4);
    if (ret < 4) {
        printf("read size failed\n");
        return ret;
    }

    file_size = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    printf("file_size = %d\n", file_size);
    memset(buf, 0, 200);


    lseek(fd, 8, SEEK_CUR);
    ret = read(fd, buf, 4);
    fmt_size = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    printf("fmt size = %d\n", fmt_size);
    memset(buf, 0, 200);
    lseek(fd, 2, SEEK_CUR);

    ret = read(fd, buf, 2);
    num_channels = (buf[1] << 8) | buf[0];
    printf("channels number = %d\n", num_channels);
    memset(buf, 0, 200);

    ret = read(fd, buf, 4);
    samplerate = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    printf("sample rate = %d\n", samplerate);
	bytesperperiod = samplerate / 1000;
	printf("bytes per period = %d\n", bytesperperiod);
    memset(buf, 0, 200);
	compare_buf = malloc(sizeof(int)*bytesperperiod);
	if (compare_buf == NULL) {
		printf("malloc compare buf failed\n");
		return -1;
	}
    lseek(fd, 6, SEEK_CUR);
    ret = read(fd, buf, 2);
    bitspersample = (buf[1] << 8) | buf[0];
    bytespersample = bitspersample / 8;
    printf("bitspersample = %d, bytespersample = %d\n", bitspersample, bytespersample);
    memset(buf, 0, 200);

    lseek(fd, fmt_size - 12, SEEK_CUR);
    ret = read(fd, buf, 4);
    data_size = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    memset(buf, 0, 200);
	
	while(data_size > 0) {
		ret = read(fd, buf, num_channels*bytespersample*bytesperperiod);
		for(i=0; i<num_channels*bytesperperiod; i++) {
			if((buf[i*bytespersample+bytespersample-1] & 0x80) == 0x80) {
				for(j=i*bytespersample; j<i*bytespersample+bytespersample; j++)
					buf[j] = ~buf[j];
			}
		}

		memset(compare_buf, 0, sizeof(int)*bytesperperiod);
		for(i=0; i<bytesperperiod; i++) {
			for(j=0; j<bytespersample; j++) {
				compare_buf[i] |= buf[i*bytespersample*2+j] << (j * 8);
			}
		}
		count++;
		sound += get_sound(compare_buf, bytesperperiod, bitspersample);
		//printf("sound = %d, count = %d\n", sound, count);
		data_size -= num_channels*bytespersample*bytesperperiod;
	}

	free(compare_buf);
    close(fd);
	if ((sound * 100 / count) > 50) {
		printf("\n\n\n\nsound is ok\n");
		return 1;
	}
	else {
		printf("\n\n\n\nsound is not ok\n");
		return 0;
	}
}

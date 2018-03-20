#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char *argv[])
{
	FILE *fp;
	uint8_t *hdcopy;
	uint8_t *plain;
	uint32_t length;

	int i, j, k, r;

	if(argc != 3)
	{
		fprintf(stderr, "Usage: %s <HD-COPY Image> <IMA plain floppy image>\n\n", argv[0]);
		fprintf(stderr, "This small tool can decompress HD-COPY image to plain floppy image.\n");
		fprintf(stderr, "It can help you to use old HD-COPY image on modern PC,\nespecially on Virtual Machine to test out old-school softwares.\n\n");
		fprintf(stderr, "Zhiyuan Wan <h@iloli.bid> 2018, License WTFPL.\nAlgorithm analyze from <https://github.com/ciel-yu/devnotes>. Thanks him!\n");
		exit(-1);
	}
	fp = fopen(argv[1], "rb");
	if(!fp)
	{
		fprintf(stderr, "Can't open source HD-COPY image!\n");
		exit(-1);
	}
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);

	hdcopy = malloc(length);
	plain = malloc(0x168000);

	fseek(fp, 0, SEEK_SET);

	fread(hdcopy, length, 1, fp);

	printf("Decoding HD-COPY Image\nInput size = %d\n", length);
	fclose(fp);

	uint8_t *actualimage;

	uint8_t *payload;

	if(hdcopy[0] == 0xff && hdcopy[1] == 0x18)
	{
		printf("That is an HD-COPY 2.0 Image\n");
		actualimage = hdcopy + 0x0e; /* 跳过标有卷标的文件头 */
		payload = actualimage + 2 + 168; /* 载荷段 */
	}
	else
	{
		printf("That is an HD-COPY 1.7 Image\n");
		actualimage = hdcopy;
		payload = actualimage + 2 + 164;
	}
	/* 开始解码 */
	int maxTrackCount = actualimage[0];
	int secPerTrack = actualimage[1];

	printf("maxTrackCount = %d, secPerTrack = %d\n", maxTrackCount, secPerTrack);
	uint8_t *dataTrack = actualimage + 2; /* 有数据的磁道表 */

	uint8_t *decode_p = plain;

	memset(plain, 0x00, 0x168000);

	for(i = 0; i < maxTrackCount; i++)
	{
		for(j = 0; j < 2; j++)
		{
			if(dataTrack[(i * 2) + j] != 0x01)
			{
				decode_p += 512 * secPerTrack;
				continue;
			}
			int dataLen = payload[0] + (payload[1] << 8);
			payload += 2;
			uint8_t escByte; /* RLE 压缩 */
			for(k = 0; k < dataLen; k++)
			{
				if(k == 0)
				{
					escByte = payload[0];
				}
				else
				{
					if(payload[k] == escByte)
					{
						k++;
						uint8_t repeatByte = payload[k++];
						int repeat = payload[k];

						for(r = 0; r < repeat; r++)
						{
							*(decode_p++) = repeatByte;
						}
					}
					else
					{
						*(decode_p++) = payload[k];
					}
				}
			}
			payload += dataLen;
		}
	}
	uint16_t secCount = plain[0x13] + (plain[0x14] << 8);
	printf("Floppy sector count = %d, fitting to %d bytes\n",
		   secCount, secCount * 512);
	printf("Decode successfully, write it to file\n");
	fp = fopen(argv[2], "wb+");
	if(!fp)
	{
		fprintf(stderr, "Can't save plain floppy image!\n");
		goto deal;
	}
	fseek(fp, 0, SEEK_SET);
	fwrite(plain, secCount * 512 > 0 ? secCount * 512 : 0x168000, 1, fp);
	fclose(fp);

deal:
	free(hdcopy);
	free(plain);
	return 0;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
这是由我（crshen）修改的DOS版本，可用TC2.0或BC3.1等编译。
由于DOS内存限制，不可能像保护模式下那样直接申请一个大内存，故数据解码需频繁进行文件读写，磁盘I/O速度自然比不上内存，效率较CMD版要差些。
*/

int main(int argc, char *argv[])
{
	FILE *fpin, *fpout;
	unsigned char *buffer, *hdcopy, *plain, *pnDataTrkMap;
	int nTrackCount, nSecPerTrack, nBytesPerTrack, nActualImgAddr, nImgDataAddr, escByte, repeatByte, repeat;
	unsigned int nDataLen;
	unsigned long nFinPos;
	int i, j, k, r;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <HD-COPY Image> <IMA plain floppy image>\n\n", argv[0]);
		fprintf(stderr, "This small tool decompress HD-COPY image to plain floppy image.\n");
		fprintf(stderr, "Therefore, you can use old HD-COPY img on modern PC or Virtual Machine.\n\n");
		fprintf(stderr, "Zhiyuan Wan <h@iloli.bid> 2018, License WTFPL.\nAlgorithm analyze from <https://github.com/ciel-yu/devnotes>. Thanks him!\n");
		fprintf(stderr, "Modified to suit old DOS by crshen <crshen@qq.com>.\n\n");
		exit(-1);
	}
	fpin = fopen(argv[1], "rb");
	if (!fpin)
	{
		fprintf(stderr, "Can't open source HD-COPY image!\n");
		exit(-1);
	}

	fpout = fopen(argv[2], "wb+");
	if (!fpout)
	{
		fprintf(stderr, "Can't save plain floppy image!\n");
		exit(-1);
	}

	buffer = malloc(2);

	fseek(fpin, 0, SEEK_SET);
	fread(buffer, 2, 1, fpin);
	if (buffer[0] == 0xff && buffer[1] == 0x18)
	{
		printf("Source img is an HD-COPY 2.0 Image\n");
		nActualImgAddr = 14; /* 跳过标有卷标的文件头 */
		nImgDataAddr = nActualImgAddr + 2 + 168; /* 磁道数据起始 */
	}
	else
	{
		printf("Source img may be an HD-COPY 1.7 Image\n");
		nActualImgAddr = 0;
		nImgDataAddr = nActualImgAddr + 2 + 164;
	}

	fseek(fpin, nActualImgAddr, SEEK_SET);
	fread(buffer, 2, 1, fpin);
	nTrackCount = buffer[0];   /* total tracks - 1 */
	nSecPerTrack = buffer[1];
	nBytesPerTrack = 512 * nSecPerTrack;
	printf("nTrackCount = %d, nSecPerTrack = %d\n", nTrackCount, nSecPerTrack);

	pnDataTrkMap = malloc(2 * (nTrackCount + 1));
	fseek(fpin, nActualImgAddr + 2, SEEK_SET);  /* tracks_map */
	fread(pnDataTrkMap, 2 * (nTrackCount + 1), 1, fpin);

	plain = malloc(nBytesPerTrack); /* 一个标准磁道容量 */
	hdcopy = malloc(nBytesPerTrack); /* 存放压缩磁道数据 */
	nFinPos = nImgDataAddr;
	printf("Working hard,please wait...\n");
	fseek(fpout, 0, SEEK_SET);
	for (i = 0; i <= nTrackCount; i++)
	{
		for (j = 0; j < 2; j++)
		{
			if (pnDataTrkMap[(i * 2) + j] != 0x01) /* 映射为空磁道 */
			{
				memset(plain, 0x00, nBytesPerTrack);
				fwrite(plain, nBytesPerTrack, 1, fpout);
				continue;
			}
			fseek(fpin, nFinPos, SEEK_SET);
			fread(buffer, 2, 1, fpin);
			nDataLen = buffer[0] + (buffer[1] << 8); /* little endian */
			memset(hdcopy, 0x00, nBytesPerTrack);
			fread(hdcopy, nDataLen, 1, fpin);
			nFinPos = nFinPos + 2 + nDataLen;
			escByte = hdcopy[0];
			for (k = 1; k < nDataLen; k++)  /* RLE压缩的磁道内容解压 */
			{
				if (hdcopy[k] == escByte)
				{
					k++;
					repeatByte = hdcopy[k++];
					repeat = hdcopy[k];
					for (r = 0; r < repeat; r++)
					{
						*(plain++) = repeatByte;
					}
				}
				else
				{
					*(plain++) = hdcopy[k];
				}
			}
			plain -= nBytesPerTrack;
			fwrite(plain, nBytesPerTrack, 1, fpout);
		}
	}

	fclose(fpin);
	fclose(fpout);
	free(hdcopy);
	free(buffer);
	free(pnDataTrkMap);
	free(plain);
	printf("Decompress operation completed.\n");
	return 0;
}


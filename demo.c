//�����Windows�Ļ�������ϵͳAPI ShellExecuteA��ͼƬ
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#define USE_SHELL_OPEN
#endif
#include "cpuimage.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//ref:https://github.com/nothings/stb/blob/master/stb_image.h
#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h" 
//ref:https://github.com/serge-rgb/TinyJPEG/blob/master/tiny_jpeg.h
#include <math.h>
#include <io.h>    
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

//��ʱ 
#include <stdint.h>
#if   defined(__APPLE__)
# include <mach/mach_time.h>
#elif defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else // __linux
# include <time.h>
# ifndef  CLOCK_MONOTONIC //_RAW
#  define CLOCK_MONOTONIC CLOCK_REALTIME
# endif
#endif
static
uint64_t nanotimer() {
	static int ever = 0;
#if defined(__APPLE__)
	static mach_timebase_info_data_t frequency;
	if (!ever) {
		if (mach_timebase_info(&frequency) != KERN_SUCCESS) {
			return 0;
		}
		ever = 1;
	}
	return;
#elif defined(_WIN32)
	static LARGE_INTEGER frequency;
	if (!ever) {
		QueryPerformanceFrequency(&frequency);
		ever = 1;
	}
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return (t.QuadPart * (uint64_t)1e9) / frequency.QuadPart;
#else // __linux
	struct timespec t;
	if (!ever) {
		if (clock_gettime(CLOCK_MONOTONIC, &spec) != 0) {
			return 0;
		}
		ever = 1;
	}
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return (t.tv_sec * (uint64_t)1e9) + t.tv_nsec;
#endif
}

static double now()
{
	static uint64_t epoch = 0;
	if (!epoch) {
		epoch = nanotimer();
	}
	return (nanotimer() - epoch) / 1e9;
};

double  calcElapsed(double start, double end)
{
	double took = -start;
	return took + end;
}



//�洢��ǰ�����ļ�λ�õı���
char  saveFile[1024];
//����ͼƬ
unsigned char * loadImage(const char *filename, int *Width, int *Height, int *Channels)
{

	return    stbi_load(filename, Width, Height, Channels, 0);
}
//����ͼƬ
void saveImage(const char *filename, int Width, int Height, int Channels, unsigned char *Output)
{

	memcpy(saveFile + strlen(saveFile), filename, strlen(filename));
	*(saveFile + strlen(saveFile) + 1) = 0;
	//����Ϊjpg
	if (!tje_encode_to_file(saveFile, Width, Height, Channels, true, Output))
	{
		fprintf(stderr, "д�� JPEG �ļ�ʧ��.\n");
		return;
	}

#ifdef USE_SHELL_OPEN 
	ShellExecuteA(NULL, "open", saveFile, NULL, NULL, SW_SHOW);
#else
	//����ƽ̨�ݲ�ʵ��
#endif
}


#ifndef ClampToByte
#define  ClampToByte(  v )  ( ((unsigned)(int)(v)) <(255) ? (v) : ((int)(v) < 0) ? (0) : (255)) 
#endif 

#define M_PI 3.14159265358979323846f

typedef struct cpu_HoughLine
{
	float Theta;
	int Radius;
	int Intensity;
	float RelativeIntensity;
} cpu_HoughLine;


typedef struct cpu_rect
{
	int  x;
	int  y;
	int  Width;
	int  Height;
} cpu_rect;

#ifndef clamp
#define clamp(value,min,max)  ((value) > (max )? (max ): (value) < (min) ? (min) : (value))
#endif 

//�ָ�·������
void splitpath(const char* path, char* drv, char* dir, char* name, char* ext)
{
	const char* end;
	const char* p;
	const char* s;
	if (path[0] && path[1] == ':') {
		if (drv) {
			*drv++ = *path++;
			*drv++ = *path++;
			*drv = '\0';
		}
	}
	else if (drv)
		*drv = '\0';
	for (end = path; *end && *end != ':';)
		end++;
	for (p = end; p > path && *--p != '\\' && *p != '/';)
		if (*p == '.') {
			end = p;
			break;
		}
	if (ext)
		for (s = end; (*ext = *s++);)
			ext++;
	for (p = end; p > path;)
		if (*--p == '\\' || *p == '/') {
			p++;
			break;
		}
	if (name) {
		for (s = p; s < end;)
			*name++ = *s++;
		*name = '\0';
	}
	if (dir) {
		for (s = path; s < p;)
			*dir++ = *s++;
		*dir = '\0';
	}
}

//ȡ��ǰ������ļ�λ��
void getCurrentFilePath(const char *filePath, char *saveFile)
{
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	splitpath(filePath, drive, dir, fname, ext);
	int n = strlen(filePath);
	memcpy(saveFile, filePath, n);
	char * cur_saveFile = saveFile + (n - strlen(ext));
	cur_saveFile[0] = '_';
	cur_saveFile[1] = 0;
}


int main(int argc, char **argv)
{
	printf("Image Processing \n ");
	printf("����:http://tntmonks.cnblogs.com/ \n ");
	printf("֧�ֽ�������ͼƬ��ʽ: \n ");
	printf("JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC \n ");

	//�������Ƿ���ȷ 
	if (argc < 2)
	{
		printf("�������� \n ");
		printf("���Ϸ��ļ�����ִ���ļ��ϣ���ʹ�������У�demo.exe ͼƬ \n ");
		printf("���Ϸ��ļ�����: demo.exe d:\\image.jpg \n ");

		return 0;
	}

	char*szfile = argv[1];
	//���������ļ��Ƿ����
	if (_access(szfile, 0) == -1)
	{
		printf("������ļ������ڣ��������� \n ");
	}

	getCurrentFilePath(szfile, saveFile);

	int Width = 0;                    //ͼƬ���
	int Height = 0;                   //ͼƬ�߶�
	int Channels = 0;                 //ͼƬͨ����
	unsigned char *inputImage = NULL; //����ͼƬָ��
	double startTime = now();
	//����ͼƬ
	inputImage = loadImage(szfile, &Width, &Height, &Channels);

	double nLoadTime = calcElapsed(startTime, now());
	printf("���غ�ʱ: %d ����!\n ", (int)(nLoadTime * 1000));
	if ((Channels != 0) && (Width != 0) && (Height != 0))
	{
		//����������ͬ���ڴ����ڴ����������
		unsigned char *outputImg = (unsigned char *)stbi__malloc(Width * Channels * Height * sizeof(unsigned char));
		if (inputImage)
		{
			//���ͼƬ���سɹ��������ݸ��Ƹ�����ڴ棬���㴦��
			memcpy(outputImg, inputImage, Width * Channels * Height);
		}
		else
		{
			printf("�����ļ�: %s ʧ��!\n ", szfile);
		}
		startTime = now();
		float arrRho[100];
		float arrTheta[100];
		int	nTNum = 200;
		int nTVal = 100;
		float Theta = 1.0f;
		CPUImageGrayscaleFilter(inputImage, outputImg, Width, Height, Width*Channels);
		CPUImageSobelEdge(outputImg, outputImg, Width, Height);
		int nLine = CPUImageHoughLines(outputImg, Width, Height, nTNum, nTVal, Theta, 100, arrRho, arrTheta);
		memcpy(outputImg, inputImage, Width * Channels * Height);
		for (int i = 0; i < nLine; i++)
		{
			if (arrTheta[i] == 90)
			{
				CPUImageDrawLine(outputImg, Width, Height, Width*Channels, (int)arrRho[i], 0, (int)arrRho[i], Height - 1, 255, 0, 0);
			}
			else
			{
				int x1 = 0;
				int y1 = (int)(arrRho[i] / fastCos(arrTheta[i] * M_PI / 180.0f) + 0.5f);
				int x2 = Width - 1;
				int y2 = (int)((arrRho[i] - x2*fastSin(arrTheta[i] * M_PI / 180.0f)) / fastCos(arrTheta[i] * M_PI / 180.0f) + 0.5f);
				CPUImageDrawLine(outputImg, Width, Height, Width*Channels, x1, y1, x2, y2, 255, 0, 0);
			}
		}
		//�����㷨
		double 	nProcessTime = now();
		printf("�����ʱ: %d ����!\n ", (int)(nProcessTime * 1000));
		//���洦����ͼƬ
		startTime = now();

		saveImage("_done.jpg", Width, Height, Channels, outputImg);
		double nSaveTime = calcElapsed(startTime, now());

		printf("�����ʱ: %d ����!\n ", (int)(nSaveTime * 1000));
		//�ͷ�ռ�õ��ڴ�
		if (outputImg)
		{
			stbi_image_free(outputImg);
			outputImg = NULL;
		}

		if (inputImage)
		{
			stbi_image_free(inputImage);
			inputImage = NULL;
		}
	}
	else
	{
		printf("�����ļ�: %s ʧ��!\n", szfile);
	}

	getchar();
	printf("��������˳����� \n");

	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/times.h>

#include <mpi.h>

#define IMG_WIDTH 1920
#define IMG_HEIGHT 1080
#define Host_ID 0

#define LAMBDA 0.633
#define PI 3.1415

#pragma pack(push,1)
typedef struct tagBITMAPFILEHEADER
{
    unsigned short bfType;
    unsigned int  bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int  bf0ffBits;
}BITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER
{
    unsigned int   biSize;
    int            biWidth;
    int            biHeight;
    unsigned short  biPlanes;
    unsigned short  biBitCount;
    unsigned int   biCompression;
    unsigned int   biSizeImage;
    int            biXPelsPerMeter;
    int            biYPelsPerMeter;
    unsigned int   biCirUsed;
    unsigned int   biCirImportant;
}BITMAPINFOHEADER;

typedef struct tagRGBQUAD
{
    unsigned char  rgbBlue;
    unsigned char  rgbGreen;
    unsigned char  rgbRed;
    unsigned char  rgbReserved;
}RGBQUAD;

typedef struct tagBITMAPINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];
}BITMAPINFO;


int main(int argc, char *argv[]) {
    int rank, proc;           //　ランク, 全プロセス数
    int name_length = 10;     //　ホスト名の長さ
    char name[name_length];  //　ホスト名
  
    MPI_Init(&argc, &argv);   // MPIの初期化
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   // ランクの取得
    MPI_Comm_size(MPI_COMM_WORLD, &proc);   // 全プロセス数の取得
    MPI_Get_processor_name(name, &name_length);   // ホスト名の取得　
    printf("start! %s : %d of %d\n", name, rank, proc);  // 結果の表示

// init MPI rank

    int summesh = IMG_WIDTH * IMG_HEIGHT;
    int iproc, Nproc, iprocINI, iprocEND;
    double *intensity;

    /* 各プロセスの繰り返し数  */
    if ( rank < summesh % proc ){
	    Nproc = summesh / proc + 1;
    }
    else{
	    Nproc = summesh / proc;
    }

    /* 各プロセスの開始点 */
    if ( summesh % proc == 0 ){
	    iprocINI = rank * Nproc;
    }
    /* 割り切れない場合 */
    /* +1をした部分まで   */
    else if ( rank < summesh % proc ){
	    iprocINI = rank * Nproc;
    }
    /* +1をした部分としてない部分の境界*/
    else if ( rank == summesh % proc ){
	    iprocINI = rank * ( Nproc + 1 );
    }
    /* +1をしてない部分*/
    else{
	    iprocINI = summesh % proc + rank * Nproc;
    }

    iprocEND = iprocINI + Nproc;

    if (rank == Host_ID) {
        intensity = (double *)malloc(sizeof(double) * (iprocEND - iprocINI) * proc);
    }

// calculation

    FILE *fpten;
    fpten = fopen("cube284.3d","rb");

    int n;
    fread(&n, sizeof(int), 1, fpten);

    int x[n], y[n];
    double z[n];
    double max = 0, min = INFINITY, mid = 0;
    clock_t startClock, endClock; 
    struct timeval startTime, endTime;

    for (int i = 0; i < n; i++) {
        int x_buf, y_buf, z_buf;
        fread(&x_buf, sizeof(int), 1, fpten);
        fread(&y_buf, sizeof(int), 1, fpten);
        fread(&z_buf, sizeof(int), 1, fpten);
        x[i] = x_buf * 40 + 540;
        y[i] = y_buf * 40 + 960;
        z[i] = z_buf * 40.0 - 10000.0;
    }

    fclose(fpten);

    if (rank == Host_ID) {
        gettimeofday(&startTime, NULL);     // 開始時刻取得
        startClock = clock();               // 開始時刻のcpu時間取得
    }

    double *intensity_proc;
    intensity_proc = (double *)malloc(sizeof(double) * (iprocEND - iprocINI));

    for (iproc = iprocINI; iproc < iprocEND; iproc++) {
        double result = 0;
        int j = iproc % IMG_WIDTH;
        int i = iproc / IMG_WIDTH;
        for (int k = 0; k < n; k++) {
            result += cos(PI * ((j - x[k]) * (j - x[k]) + (i - y[k]) * (i - y[k])) / (LAMBDA * z[k]));
        }
        intensity_proc[iproc - iprocINI] = result;
    }

// time

    if (rank == Host_ID) {
        gettimeofday(&endTime, NULL);       // 終了時刻取得
        endClock = clock();                 // 終了時刻のcpu時間取得
    }

    if (rank == Host_ID) {
        time_t diffsec = difftime(endTime.tv_sec, startTime.tv_sec);    // 秒数の差分を計算
        suseconds_t diffsub = endTime.tv_usec - startTime.tv_usec;      // マイクロ秒部分の差分を計算
        double realsec = diffsec+diffsub*1e-6;                          // 実時間を計算
        double cpusec = (endClock - startClock)/(double)CLOCKS_PER_SEC; // cpu時間を計算
        double percent = 100.0 * cpusec / realsec;                      // 使用率を100分率で計算
        printf("1ランクの計算時間:\nCPU使用率%f %%\n実行時間%f s\nCPU時間 %f s\n", percent, realsec, cpusec);     // 表示
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == Host_ID) {
        gettimeofday(&startTime, NULL);     // 開始時刻取得
        startClock = clock();               // 開始時刻のcpu時間取得
    }

    MPI_Gather(intensity_proc, iprocEND - iprocINI, MPI_DOUBLE, intensity, iprocEND - iprocINI, MPI_DOUBLE, Host_ID, MPI_COMM_WORLD);
    free(intensity_proc);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == Host_ID) {
        gettimeofday(&endTime, NULL);       // 終了時刻取得
        endClock = clock();                 // 終了時刻のcpu時間取得
    }

    if (rank == Host_ID) {
        time_t diffsec = difftime(endTime.tv_sec, startTime.tv_sec);    // 秒数の差分を計算
        suseconds_t diffsub = endTime.tv_usec - startTime.tv_usec;      // マイクロ秒部分の差分を計算
        double realsec = diffsec+diffsub*1e-6;                          // 実時間を計算
        double cpusec = (endClock - startClock)/(double)CLOCKS_PER_SEC; // cpu時間を計算
        double percent = 100.0 * cpusec / realsec;                      // 使用率を100分率で計算
        printf("同期にかかる時間:\nCPU使用率%f %%\n実行時間%f s\nCPU時間 %f s\n", percent, realsec, cpusec);     // 表示
    }

// make picture mono color

    if (rank == Host_ID) {
        for (int i = 0; i < IMG_WIDTH; i++) {
            for (int j = 0; j < IMG_HEIGHT; j++) {
                if (intensity[i * IMG_WIDTH + j] > max) max = intensity[i * IMG_WIDTH + j];
                if (intensity[i * IMG_WIDTH + j] < min) min = intensity[i * IMG_WIDTH + j];
            }
        }
        mid = (max + min) / 2;
    }

// write bitmap

    if (rank == Host_ID) {
        unsigned char img[IMG_WIDTH][IMG_HEIGHT];
        for (int i = 0; i < IMG_WIDTH; i++) {
            for (int j = 0; j < IMG_HEIGHT; j++) {
                img[i][j] = intensity[i * IMG_WIDTH + j] < mid ? 0 : 255;
            }
        }

        FILE *fp = fopen("image.bmp","wb");

        BITMAPFILEHEADER BmpFileHeader;
        BITMAPINFOHEADER BmpInfoHeader;
        RGBQUAD RGBQuad[256];
        BmpFileHeader.bfType = 19778;
        BmpFileHeader.bfSize = 14 + 40 + 1024 + (IMG_WIDTH * IMG_HEIGHT);
        BmpFileHeader.bfReserved1 = 0;
        BmpFileHeader.bfReserved2 = 0;
        BmpFileHeader.bf0ffBits = 14 + 40 + 1024;

        BmpInfoHeader.biSize = 40;
        BmpInfoHeader.biWidth = IMG_WIDTH;
        BmpInfoHeader.biHeight = IMG_HEIGHT;
        BmpInfoHeader.biPlanes = 1;
        BmpInfoHeader.biBitCount = 8;
        BmpInfoHeader.biCompression = 0L;
        BmpInfoHeader.biSizeImage = 0L;
        BmpInfoHeader.biXPelsPerMeter = 0L;
        BmpInfoHeader.biYPelsPerMeter = 0L;
        BmpInfoHeader.biCirUsed = 0L;
        BmpInfoHeader.biCirImportant = 0L;

        for (int i = 0; i < 256; i++) {
            RGBQuad[i].rgbBlue = i;
            RGBQuad[i].rgbGreen = i;
            RGBQuad[i].rgbRed = i;
            RGBQuad[i].rgbReserved = 0;
        }
    
        fwrite(&BmpFileHeader.bfType, sizeof(BmpFileHeader.bfType), 1, fp);
        fwrite(&BmpFileHeader.bfSize, sizeof(BmpFileHeader.bfSize), 1, fp);
        fwrite(&BmpFileHeader.bfReserved1, sizeof(BmpFileHeader.bfReserved1), 1, fp);
        fwrite(&BmpFileHeader.bfReserved2, sizeof(BmpFileHeader.bfReserved2), 1, fp);
        fwrite(&BmpFileHeader.bf0ffBits, sizeof(BmpFileHeader.bf0ffBits), 1, fp);
        fwrite(&BmpInfoHeader, sizeof(BmpInfoHeader), 1, fp);
        fwrite(&RGBQuad[0], sizeof(RGBQuad[0]), 256, fp);
    
        for (int j = 0; j < IMG_HEIGHT; j++) {
    	    for (int i = 0; i < IMG_WIDTH; i++) {
                fwrite(&img[i][j], sizeof(img[i][j]), 1, fp);
    	    }
        }
    
        fclose(fp);
        free(intensity);
    }
    MPI_Finalize();   // MPIの終了処理
    return 0;
}

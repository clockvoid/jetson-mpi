#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <sys/time.h>
#define Host_ID 0

int main(int argc, char *argv[]){
  int rank, proc;           //　ランク, 全プロセス数
  int name_length = 10;     //　ホスト名の長さ
  char name[name_length];  //　ホスト名
  struct timeval startTime, endTime;

  MPI_Init(&argc, &argv);   // MPIの初期化
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   // ランクの取得
  MPI_Comm_size(MPI_COMM_WORLD, &proc);   // 全プロセス数の取得
  MPI_Get_processor_name(name, &name_length);   // ホスト名の取得　

  int N;
  N = 1e8;

  int iproc, Nproc, iprocINI, iprocEND;
  int Sumproc, Sum;

  /* 各プロセスの繰り返し数  */
  if ( rank < N % proc ){
    Nproc = N / proc + 1;
  }
  else{
    Nproc = N / proc;
  }

  /* 各プロセスの開始点 */
  if ( N % proc == 0 ){
    iprocINI = rank * Nproc;
  }
  /* 割り切れない場合 */
  /* +1をした部分まで   */
  else if ( rank < N % proc ){
    iprocINI = rank * Nproc;
  }
  /* +1をした部分としてない部分の境界*/
  else if ( rank == N % proc ){
    iprocINI = rank * ( Nproc + 1 );
  }
  /* +1をしてない部分*/
  else{
    iprocINI = N % proc + rank * Nproc;
  }

  iprocEND = iprocINI + Nproc;

  printf("rank = %d, iprocINI=%d, iprocEND=%d\n",rank, iprocINI, iprocEND);

  /* シグマの並列計算 */
  if (rank == Host_ID)
    gettimeofday(&startTime, NULL);     // 開始時刻取得
  Sumproc = 0; Sum = 0;
  for ( iproc = iprocINI; iproc < iprocEND; iproc++){
     Sumproc += ( iproc + 1 );
  }
  if (rank == Host_ID)
    gettimeofday(&endTime, NULL);       // 終了時刻取得

  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Reduce(&Sumproc, &Sum, 1,MPI_INT,MPI_SUM,Host_ID,MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == Host_ID) {
    time_t diffsec = difftime(endTime.tv_sec, startTime.tv_sec);    // 秒数の差分を計算
    suseconds_t diffsub = endTime.tv_usec - startTime.tv_usec;      // マイクロ秒部分の差分を計算
    double realsec = diffsec+diffsub*1e-6;                          // 実時間を計算
    printf("実行時間%f s\n", realsec);     // 表示
    printf("Sum = %d\n", Sum);
  }

  printf("%s : %d of %d\n", name, rank, proc);  // 結果の表示
  MPI_Finalize();   // MPIの終了処理
  return 0;
}

# OpenMPIとCudaを一つのプログラムの中で共存させて動かすためのドキュメント

基本的には[この](https://anhnguyen.me/2013/12/how-to-mix-mpi-and-cuda-in-a-single-program/)ページのようにする．

仕組みとしては，nvccにOpenMPIで使うライブラリのパスとDynamic Libraryのパスを渡して，`-lmpi`を適用してビルドするだけ．

ここでできたバイナリを`mpirun -np 2 nvprof ./a.out`のようにしてnvprofでプロファイリングしながら実行することが可能である．

## UbuntuでのOpenMPIのインストール先
Ubuntuでは上サイトのように単純なパスにインストールされていなかった．
Include先は`/usr/lib/aarch64-linux-gnu/openmpi/include`，Dynamic Libraryは`/usr/lib/aarch64-linux-gnu/openmpi/lib`にインストールされていたため，この環境ではビルドスクリプトは以下のようになる．

```bash
nvcc -I/usr/lib/aarch64-linux-gnu/openmpi/include -L/usr/lib/aarch64-linux-gnu/openmpi/lib -lmpi $1.cu -o $1
```

## `build-cuda`の使い方
```bash
build-cuda {コンパイルしたいファイルの名前（拡張子なし）}
```

とするのみで，引数に指定した名前のバイナリが出来上がる．

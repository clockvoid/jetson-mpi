# Jetson-mpi
Jetson NanoでOpenMPIを用いた計算機クラスタを作る作戦

## 環境構築
### OpenMPI
まず，Jetsonに初期搭載のUbuntuにはOpenMPIがはじめからインストールされている．
基本的には，OpenMPIをインストールするところから始めるが，その際には以下のコマンドを使うことになる
```bash

$ sudo apt-get install openmpi-doc openmpi-bin libopenmpi-dev
```

これを実行するとdocのみがインストールされる．

これをすべてのJetsonで実行する．

### OpenSSH
続いて，すべてのJetsonで相互接続するためにすべての環境でSSHをインストールし，RSAでパスワード無しでログインできるようにする．
パスワード無しでログインできるようにするためには`ssh-keygen`でパスワードを入力するときに何も入力せずにEnterを押すだけで良い．

作成した公開鍵をゲスト側のノードの`authrizedkeys`にコピーする．それだけ．

### NFS
続いて，複数のJetson Nanoで実行結果を共有できたほうがやりやすいため，ファイル共有プロトコル，NFSを使ってホストに作成したディレクトリをすべてのゲストでマウントする．
NFSに関しては初めて使ったので，以下のサイトを参照．

[NFSによるファイルの共有](http://tanizawade.gozaru.jp/topic/common_disk.html)

なお，`exports`ファイルには，以下のように書けば良い

```exports
/{target} 	*(rw,sync,no_root_squash)
```

host名は`*`とかくととりあえずすべてのゲストから読めるようになる．

UbuntuではNFSはnfsdというデーモンで管理されるので`systemctl`を適当に再起動して設定を適用させる．

ゲスト側ではマウントするだけではなく，`/etc/fstab`に以下のような設定を追記する．

```fstab
10.66.60.148:/{target} /{target} nfs defaults 0 0
```

これだけで起動時に自動的に再度マウントされる．

## OepnMPIのホストの設定
OpenMPIでは`mpirun`というコマンドを使って並列数とそのときに使うデバイスの設定が可能．その際には`--hostfile`というオプションでファイルを指定することで，複数のデバイスで実行する設定をすることが可能である．

hostファイルは以下のように書く．

```host
masudalab-desktop slots=4
jetson-node2 slots=4
```

一行目にホストのホスト名を記述する．ホスト名は`/etc/hosts`で設定しておく．`slot`はそのデバイスでの最大並列数を記述する．デバイスの物理的なコア数を超えて指定すると多分パフォーマンスは落ちる．

## コンパイル
コンパイルにはC言語でプログラムを記述した場合は`mpicc`を使う．C++では`mpic++`を使う．
プログラムは普通のプログラムに，`main()`関数のはじめに

```c
MPI_Init(&argc, &argv);   // MPIの初期化
MPI_Comm_rank(MPI_COMM_WORLD, &rank);   // ランクの取得
MPI_Comm_size(MPI_COMM_WORLD, &proc);   // 全プロセス数の取得
MPI_Get_processor_name(name, &name_length);   // ホスト名の取得
```
を記述して，初期化とそのプロセスのランク（全プロセス中での何番目のプロセス化を示す数字），全プロセス数，そのプロセスが実行されているホストの名前を取得することが可能．

この情報を使って逐次的な処理並列化する．

続いて，プログラムの最後に

```c
MPI_Finalize();   // MPIの終了処理
```

を記述するだけでOpenMPIのプログラムとして動作する．

これを

```bash
mpicc sample.c -o sample
```

としてコンパイルする．

### 実行
最後に，`mpirun`を使って実行する．基本的には上述のようにしてコンパイルし，

```bash
mpirun --hostfile host -np 2 sample
```

とかく．`--hostfile`でノード一覧を書いたファイルを指定し，`-np`で並列数を指定する，最後にコンパイルしたバイナリを渡すだけ．

### 実際の調整
実際にはランクで挙動を変えたりして，最後に同期を取るなどの調整を行う必要がある．

よくあるのは`MPI_Barrier`とか`MPI_Reduce`とか`MPI_Gather`とか`MPI_Allgather`とかを使うことが多い．

すべての関数ではCommunicatorとか言うものを使うが，これは多分同期を取る際の通信処理の種類を指定できるものだと思う．

基本的には全体で使える`MPI_COMM_WORLD`をよく使う．

#### `MPI_Barrier`
この関数は以下の引数を取る

- Communicator

この関数は指定したCommunicatorでのすべての処理が終わるのを待つ．

#### `MPI_Reduce`
この関数はすべてのノードで出た値を何らかの処理を行い，同期する関数．

この関数は以下の引数を取る．

- Senderの変数へのポインタ
- Receiverの変数へのポインタ
- コピーする変数の要素数（リストでも良いため）
- コピーする変数の型
- 同期に使うホストのランク
- Communicator

#### `MPI_Gather`
この関数はすべてのノードで出た値を一つのノードに集約する関数．

この関数は以下の引数を取る．

- Senderの変数へのポインタ
- Senderの変数の要素数
- Senderの変数の要素の型
- Receiverの変数へのポインタ
- Receiverの変数の要素数
- Receiverの変数の要素の型
- 同期に使うホストのランク
- Communicator

#### `MPI_Allgather`
この関数は`MPI_Gather`の一つのノードではなくすべてのノードにすべてのノードの結果をコピーする版．

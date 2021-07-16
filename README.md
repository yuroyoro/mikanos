# MikanOS
[ゼロからのOS自作入門](https://zero.osdev.jp/) を写経するリポジトリ。

## Memo

### 第1章 PCの仕組みと ハローワールド

Macで開発するので開発環境作るよ。
VSCodeとDockerで環境構築余裕でした。先人の知恵ありがてえっ……！！

- [Docker ではじめる "ゼロからのOS自作入門"](https://zenn.dev/sarisia/articles/6b57ea835344b6)
- [「ゼロからのOS自作入門」の副読本的記事](https://zenn.dev/karaage0703/articles/1bdb8930182c6c)

テンプレートのリポジトリ( [sarisia/mikanos\-devcontainer: VSCode devcontainer for developing MikanOS](https://github.com/sarisia/mikanos-devcontainer) ) をcloneしてVSCodeで起動すると、Remote Containerが起動して一通りの開発環境が揃っている。最高

コンテナ内でdisk imageを作ってQEMUで起動、Mac側ではXQuartzを起動しておき、コンテナ内でQEMUでhello.efiを起動してハローワールド完了

### 第2章 EDK II入門とメモリマップ

これから本格的に手を動かして写経していくので、そのための環境を整える。
書籍の内容を確認しながら、オリジナルのMikanOSのリポジトリのコミットを写経していくので、dev-container内にオリジナルのMikanOSをcloneしておく。
また、diff-highlightやtigなどgit周りを便利に使えるようにdev-containerのDockerfileに追記した

vscodeで `Uefi.h` などから補完が効くように、 `.vscode/c_cpp_properties` に `${HOME}/edk2/MdePkg/Include/**` にパスを通すように設定しておくよ。

#### 2.2 EDK II でハローワールド(osbook_day02a)

手順通りに進めれば問題ないが、 `$HOME/edk2` に作る `MikanLoaderPkg` へのSymlinkはコンテナ起動ごとに削除されてしまうので、Dockerfile内で先に作って置くことにした。
また、 `edk2/Conf` も消えてしまうので、リポジトリに入れておいてこれもSymlinkすることにした

```Dockerfile
# create a symlink to MikanLoaderPkg in my repository
RUN cd /home/${USERNAME}/edk2 &&  ln -s /workspaces/mikanos/MikanLoaderPkg ./

# create a symlink to my Conf/
RUN cd /home/${USERNAME}/edk2 && rm -rf ./Conf && ln -s /workspaces/mikanos/edk2/Conf ./

```

#### 2.5 メモリマップの取得(osbook_day02b)

オリジナルのタグ [osbook_day02b](https://github.com/uchan-nos/mikanos/tree/osbook_day02b) のコミットを参考に、メモリマップを取得してファイルに書き出す実装を `Main.c` に書いていく。

注意点として、 `Main.c` の他に `Loader.inf` で使用するProtocolを追記しないとビルドに失敗するぞ。
[get memory map without error checking · uchan\-nos/mikanos@a695aa4](https://github.com/uchan-nos/mikanos/commit/a695aa4c2b43f1edd45cbca6e05b0bb3da72bf58#diff-42d8ec170a53cc42f50f259e3420e36b4ea931bc7b0ba917e3f122bf7e95d033R24-R26)

あとは、 書籍の手順通りに `run_qumu.sh` でビルドした `Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi` を起動して、イメージにファイルが書き込まれているか確認。

### 第3章 画面表示の練習とブートローダ

#### 3.3 初めてのカーネル(osbook_day03a)

[osbook_day03a](https://github.com/uchan-nos/mikanos/tree/osbook_day03a) を見ながら写経。

まずは `kernel/main.cpp` を作成してコンパイル/リンクする。ちょくでclang叩くのがダルいのでMakefile書こうと思ったけど、次の章でやるのね
あとはブートローダーがカーネルを読むように修正する。むしろこっちがメインの内容。
オリジナルでは `UefiMain` に処理を書いていたけど、適当に関数に分けて書いてみた

Loader.infに `gEfiFileInfoGuid` を追記するのを忘れないように。

```
[Guids]
  gEfiFileInfoGuid
```

一度バグがあってブートローダーが無限ループに陥ったあと、何度実行してもブートローダーが起動しない(Hello worldすら出ない)状況になり、コンテナごと再起動すると回復した。ローレイヤー怖い

#### 3.4 ブートローダからピクセルを描く(osbook_day03b)

特になし

#### 3.5 カーネルからピクセルを描く(osbook_day03c)

`$HOME/osbook/devenv/buildenv.sh` で `$CPPFLAGS` を設定しているが、zshの場合はクォートで文字列扱いになるので、 `clang++ $CPPFLAGS  ...` は `$CPPFLAGS` の展開に失敗する。

`buildenv.sh` を書き換えるか、bashを使ってコンパイル/リンクして対処する。
 `buildenv.sh` はコンテナに焼き込んであるので起動時に初期化されてしまう。bash使うのが無難。というかはよMakefile書きたい

#### 3.6 エラー処理をしよう(osbook_day03d)

いままでは `UefiMain` でエラーのときは gotoで飛ばすようにしていたが、 `Halt` 関数を導入した

### 第4章 ピクセル描画とmake入門

#### 4.1 make入門(osbook_day04a)

ようやくMakefile。ただしkernelのビルドのみ。buildenv.shとかもまとめて設定するようにあとでやろう

#### 4.2 ピクセルを自在に描く(osbook_day04b)

`main.cpp` から `frame_buffer_config.hpp` をincludeしているので、include pathを通す必要がある……と思ったら、`MikanLoaderPkg` と `kernel` でsymlinkしていた。

#### 4.3 C++ の機能を使って書き直す(osbook_day04c)

c++むずかしいです

#### 4.5 ローダを改良する(osbook_day04d)

ELF形式のローダーの話。例によって `elf.hpp` はsymlinkしておく。


### 第5章 文字表示とコンソールクラス

#### 5.1 文字を書いてみる(osbook_day05a)

コンソールに文字を吐くためにフォントデータから1ピクセルづつ描画するやつ。Nand2Tetrisでもやったけど、あちらのグラフィックスばモノクロのbitmapだったが、こちらは256色。
まあ大した差ではないけど。

そろそろ、 ブートローダーをビルドするのにいちいち `source ~/edk2/edksetup.sh && build` とかやるのがしんどくなってきたので、 Makefileでブートローダーとカーネルを一度にビルドできて、qemuで起動できるようにした

#### 5.2 分割コンパイル(osbook_day05b)

`kernel/main.cpp` を分割するだけ。


#### 5.3 フォントを増やそう(osbook_day05c)

フォントデータをバイナリに埋め込むようにする。
フォントデータの `kernel/hankaku.txt` とそこからベタバイナリを生成する `tools/makefont.py` は流石に写経するのはダルいのでコピーaだけではコンパイルが通らない。 `error.hpp`

#### 5.4 文字列描画とsprintf()(osbook_day05d)

Newlibを使ってsprintfを使えるようにするやつ。
Newlibで必要なカーネルの機能を `newlib_support.c`  に書いていくのだけど、いまのところメモリ確保用の  `sbrk` さえあればよいっぽい。
あとでカーネルがメモリ管理できるようになったらbrkシステムコールに置き換えるのかな

#### 5.5 コンソールクラス(osbook_day05e)

文字出力用のコンソールクラスの実装。25行 x 80文字で文字出力し、スクロールも実装する。

#### 5.6 printk()(osbook_day05f)

printk関数の実装。特に問題はなかった。
そういえば配列宣言を利用してグローバル変数用のメモリ確保するやつはNand2Tetrisでもやったっけ(あちらはアセンブリだったけど)

### 第6章 マウス入力とPCI

#### 6.1 マウスカーソル(osbook_day06a)

デスクトップとマウスカーソルの描画を実装する。
`FillRectangle` と `DrawRectangle` は `PixelWriter` のメンバー関数にしてみた。

#### 6.3 PCIデバイスの探索(osbook_day06b)

ひたすら `pci.cpp` などを写経していくぞ。

`MakeAddress` の実装で、 指定位置までビットシフトしてor撮ってるけど、 `bus` や `device` が上限超えてたら範囲外のbit立ってしまうバグがあるな。
こういうのはassertとかで潰すのかな。カーネルだとどうすればいいんだろうな?

`main.cpp` から 配置newで定義した `void* operator new(size_t size, void* buf) ` の定義が消えている。
`~/osbook/devenv/x86_64-elf/include/c++/v1/new` にあるので不要ということだと思うけど、どこからこれがincludeされるようになったかはわからん( ꒪⌓꒪)

#### 6.4 ポーリングでマウス入力(osbook_day06c)

いよいよマウスを動かす実装をするぞ。まず USBホストコントローラドライバとマウスのクラスドライバを `~/mikanos/kernel/usb` からコピーしてくる。
中身をちょっと覗いてみたけど、理解するのは諦めた。
単にコピーしてくるだけではコンパイルが通らない。

まず、 `Error` 型の定義が変わっている(ファイル名と行数を持つようになった)ので `error.hpp` を修正していく。
ファイル名と行数を含めるためのマクロ `MAKE_ERROR` も定義する。それに伴い、 `Error` を使っている場所をマクロに置き換えていく。

次に、 `Log` 関数を定義してカーネルからログ出力できるようにする。

さらに、 メモリマップドなレジスタ操作をするための型を `usr` 配下で参照しているので、 `kernel/register.hpp` で色々と型を追加する。
中はテンプレートを駆使して指定した型のサイズでレジスタに読み書きできるようにした `MemMapRegister` と 配列をイテレーションする `ArrayWrapper` が定義されている。

必要なシステムコールが増えたのか、 `newlib_support` に `_exit` `getpid` `kill` を追加する。

まだコンパイルが通らない。

```
ld.lld: error: undefined symbol: std::get_new_handler()
>>> referenced by new.cpp
>>>               new.cpp.o:(operator new(unsigned long)) in archive /home/vscode/osbook/devenv/x86_64-elf/lib/libc++.a

ld.lld: error: undefined symbol: posix_memalign
>>> referenced by new.cpp
>>>               new.cpp.o:(operator new(unsigned long, std::align_val_t)) in archive /home/vscode/osbook/devenv/x86_64-elf/lib/libc++.a

ld.lld: error: undefined symbol: std::get_new_handler()
>>> referenced by new.cpp
>>>               new.cpp.o:(operator new(unsigned long, std::align_val_t)) in archive /home/vscode/osbook/devenv/x86_64-elf/lib/libc++.a

ld.lld: error: undefined symbol: posix_memalign
>>> referenced by new.cpp
>>>               new.cpp.o:(operator new(unsigned long, std::align_val_t)) in archive /home/vscode/osbook/devenv/x86_64-elf/lib/libc++.a
```

なんかlibc++がどうの言っており、オリジナルのコミットを見ると [link libc\+\+ statically · uchan\-nos/mikanos@a92f5ba](https://github.com/uchan-nos/mikanos/commit/a92f5ba79268b10a2a674e15baf690e1788d6b93) とあるので、 `newlib_support` と同じノリでlibc++の機能を追加していくっぽい。

```
ld.lld: error: undefined symbol: __cxa_pure_virtual
>>> referenced by hid.cpp
>>>               usb/classdriver/hid.o:(vtable for usb::HIDBaseDriver)
```

あとなんか、 `__cxa_pure_virtual` というしんぼるが ね―よってエラーも出ていたので、 `main.cpp` に追加する。
このあたりの話っぽい。

[自作OSでC\+\+を使う方法 \- Kludge Factory](https://tyfkda.github.io/blog/2014/03/10/use-cpp.html)

ここまでやって、ようやく取り込んだusbドライバがビルドできるようになった。

まず、 マウスの描画処理を `MouseCursor` クラスに実装する。 `mouse.hpp` と `mouse.cpp` を追加して `main.cpp` から使うようにする。
また、`pci.hpp` と `pci.cpp` でクラスコード周りやPCI コンフィギュレーション空間のBARを読み書きする実装を足していく。

`error.hpp` はいわゆるEitherだよな。mapとか色々と足したくなる。

あとは、 `main.cpp` にxHCの初期化やマウスイベントを受け取る関数の設定をして、ループの中で `ProcessEvent` を呼び出してxHCに溜まったイベントを処理するようにする。
いわゆるイベントループ方式。

### 第7章 割り込みとFIFO

割り込み〜。

#### 7.1 割り込み(osbook_day07a)

解説を読みながら、 `git diff osbook_day06c..osbook_day07a` でdiffに目を通していく。

- `interupt.hpp` `interupt.cpp`
  - 割り込み関連のデータ構造( `InterruptDescriptor` `InterruptDescriptorAttribute` )
  - `SetIDTEntry` :  割り込みハンドラの設定
  -  `NotifyEndOfInterrupt` : `0xfee000b0` へ書き込んで割り込み終了をcpuへ通知する
- `pci.hpp` `pci.cpp` : MSI割り込みの設定を行うため、PCIコンフィグレーションのcapabilty headerを読み書きするための関数を実装している。正直よくわからないです( ꒪⌓꒪)
- `asmfunc.h` `asmfunc.asm` :
  - `GetCS` : コードセグメントを取得する。 csレジスタの値をaxに設定することでcsの値が戻り地になる(axはraxの下位16it)
  - `LoadIDT` :  IDTの設定を行う。スタックトップにIDTのサイズとアドレスを書き込んで `lidt` を読んでIDTの位置をCPUに設定する
- `main.cpp`
  - `IntHandlerXHCI` : XCHIからの割り込みに対応する割り込みハンドラ。
  - main内で `SetIDTEntry` で上記の `IntHandlerXHCI` のアドレス( `reinterpret_cast<uint64_t>(IntHandlerXHCI)` )をIDTに設定している
  - `pci::ConfigureMSIFixedDestination` で割り込みを `InterruptVector::kXHCI` の割り込みベクタに発生させる設定を行っている。
  - `__asm__("sti")` で割り込みフラグをCPUに設定している。割り込みフラグ設定でCPUが割り込みに応答するようになる。

`kernel/usb/xhci/xhci.cpp` がひっそりと修正されているので注意

```diff
diff --git a/kernel/usb/xhci/xhci.cpp b/kernel/usb/xhci/xhci.cpp
index 3b597d4..621f4c0 100644
--- a/kernel/usb/xhci/xhci.cpp
+++ b/kernel/usb/xhci/xhci.cpp
@@ -66,7 +66,7 @@ namespace {
     }

     int msb_index;
-    asm("bsr %1, %0"
+    __asm__("bsr %1, %0"
         : "=r"(msb_index) : "m"(value));
     return msb_index;
   }
```

#### 7.7 割り込みハンドラの高速化(osbook_day07b)

queueを使った割り込みの実装。
`main` 関数が長くなってきたのでリファクタリングしたい。オリジナルのコードだと最終的には処理ごとに関数に分かれているのでどこかでリファクタリングするんだろうがj;w

### 第8章 メモリ管理

#### 8.2 UEFIメモリマップ(osbook_day08a)

ブートローダーからUEFI BIOSで取得したメモリマップをkernelに渡すようにする。


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
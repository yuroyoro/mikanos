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


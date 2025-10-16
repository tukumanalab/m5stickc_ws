# M5StickCPlus 1.1を使った初等部向けワークショップ

## つくまなPCのセットアップ

- 新規アカウント arduino を作成する。(Arduino IDEが日本語フォルダ名をサポートしていないため)
  - スタート > 設定 > アカウント > 他のユーザー を選択。
  - [アカウントの追加]を押す。
  - [このユーザーのサインイン情報がありません]を選ぶ。
  - [Microsoft アカウントを持たないユーザーを追加する]を選ぶ。
  - このPCを使うのはだれですか? に arduino と入力する。パスワードはなし。[次へ]を押す。
- PCからサインアウトして、arduino でサインインする。

## gemini-cliのインストール

- https://nodejs.org/ja/ より Node.js をダウンロードしてインストールする。
- Node.js用コマンドプロンプト`Node.js command prompt`を立ち上げ、 npm install -g @google/gemini-cli を実行する。
- コマンドプロンプトより、 gemini と実行する。
- 1. Login with Google を選択する。
- ブラウザが開くので、tukumanalab@gmail.com でログインする。
- プロンプト画面が開けばセットアップ完了です。

## Arduino IDEのインストール

- https://arduino.cc/en/software/ より Arduino IDE をダウンロードしてインストールする。このとき「このコンピューターを使用しているすべてのユーザー用にインストールする」を選ぶ。
- ネットワーク関連などの許可を求められたら、「許可」を選択する。
- インストールした Arduino IDE を起動する。
- File > Preferences で Language を「日本語」に変更し、OK ボタンを押す。
- ファイル > 基本設定 > 追加のボードマネージャのURL に https://espressif.github.io/arduino-esp32/package_esp32_index.json を追加して OK ボタンを押す。
- ツール > ボード > ボードマネージャ を選び、esp32 で検索して、「esp32 by Espressif Systems」をインストールします。
- ツール > ボード > esp32 > M5StickCPlus を選択します。
- スケッチ > ライブラリをインクルード > ライブラリを管理 を選び、M5StickCPlus で検索して、「M5StickCPlus by M5Stack」をインストールします。このとき、依存しているライブラリは、「全てをインストール」します。
- ファイル > スケッチ例 > (カスタムライブラリのスケッチ例の下の)M5StickCPlus > Games > FlappyBird を選択。
- M5StickCPlus を USB ケーブルで接続する。
- FlappyBird のプログラムが開いている Arduino IDE を選び、ツール > ポート > COM3(接続しているUSBポート) を選択。
- 左上の「検証」を選んでコンパイルする。
- その隣の「書き込み」を選び、プログラムを M5StickCPlus に送る。
- ゲームが起動すれば成功です。
- ファイル > Save を選択し、FlappyBird のブログラムを ドキュメント > Arduino の下に保存します。

# Gemini CLI を使ってゲームを作成

- コマンドプロンプトを起動し、 cd Documents¥Arduino と実行して、Arduino フォルダに移動する。
- gemini と実行する。
- プロンプトに「M5StickCPlusで動くスロットマシンのプログラムを slot_machine フォルダ以下に作ってください。必要なら FlappyBird 以下のプログラムを参考にしてください。」と入力してください。
- Arduino IDE で ファイル > 開く を選び、action_game/action_game.ino を開きます。
- 左上の「書き込み」を選び、できたゲームを M5StickCPlus に送ってゲームを確認してください。

もしエラーがでたら、
- エラーメッセージをコピーして、Gemini CLI に貼り付ければ、直してくれます。

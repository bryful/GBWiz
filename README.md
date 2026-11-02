# GBWiz

GB / GBC対応のWindowsプレビュー版。既存のGambatte C++コアを使用します。
Android開発版も追加しました。ビルド・端末操作は[Android説明書](android/README.md)を参照してください。
GB版外伝1〜3は起動・リセット時のロゴを自動スキップします。[ゲーム別対応状況](docs/game-support.md)を参照してください。
ロゴスキップは機能メニューでON／OFFできます。GB版外伝1～3、GBC版Wizardry 1～3、GBC版DQ3は[倍速＋等速音声](docs/turbo-audio.md)に対応しています。DQ3は全4倍率でフィールド移動・戦闘勝利・曲の復帰まで確認しています。
外伝I〜III・GBC版I〜III・GBC版DQ3の全7作品が[戦闘報酬の経験値・お金倍率](docs/reward-support.md)に対応しています。通常は×1、左右操作で×8～×256を独立して選べます。
外伝1〜3・GBC版Wizardry1〜3の[新規作成ボーナス最大化](docs/creation-support.md)もサポートメニューからON／OFFできます。
戦闘中を除き、[対象キャラクターを選んで全呪文を習得](docs/spell-support.md)させることもできます。
外伝I〜IIIとGBC版Wizardry I〜IIIでは、対象キャラクターの[属性を善・中立・悪へ変更](docs/alignment-support.md)できます。通常セーブにも反映します。
外伝1〜3・GBC版Wizardry1〜3の[善悪混合パーティー](docs/party-support.md)を全体設定でON／OFFできます。キャラクターの属性は維持します。[作品別の対応状況と検証範囲](docs/porting-status.md)を参照してください。
外伝2・3とGBC Wizardry 1〜3のMP消費なしと、GBC Wizardry 1〜3のロゴスキップも使用できます。

## ビルド

Visual Studioの「C++によるデスクトップ開発」とCMakeを使用します。
この環境では`powershell -ExecutionPolicy Bypass -File .\build.ps1`でもビルドできます。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\GBWiz.exe
```

起動するとGB / GBCのROM選択画面を表示します。初期フォルダは実行ファイル横の`Rom`です。
コマンドラインでも指定できます。

```powershell
.\build\Release\GBWiz.exe .\.rom\wizardrygaiden1.gb
.\build\Release\GBWiz.exe .\.rom\DQ3.gbc --smoke
```

`--smoke`は保存ファイルに触れず180フレーム実行し、映像の変化とステートの復元を検証します。
GB / GBC判定はROMヘッダーで行います。拡張子やハッシュには依存しません。

## 現在の操作

| 操作 | キー |
|---|---|
| 十字キー | 矢印 |
| A / B | X / Z |
| Start / Select | Enter / 右Shift |
| スロット1保存 / 復元 | F5 / F8 |
| 機能メニュー / 閉じる | F1 または Esc |

メニューでは上下で項目、左右でスロット（1〜5）とAB入れ替えを変更し、Enterで決定します。
「ROM選択」から起動中に別のGB / GBC ROMへ切り替えできます。
選択キャンセル・ROM読込失敗・セーブ不整合時は元のゲームを保持します。
切り替え前に通常セーブを保存し、切り替え先のROM名に対応するセーブを読み込みます。
「リセット」は選択すると確認画面なしで即実行し、通常セーブを保持して起動し直します。
リセット専用のコントローラーボタン割り当てはありません。
AB入れ替えはキーボードとコントローラーのゲーム入力に共通で適用します。
メニュー操作の決定・戻るは入れ替えに影響されません。

Xbox / Pro ControllerはSDL2の標準マッピングを使用します。十字キー、Start、Selectに対応し、
NES配列として右のフェイスボタンがA、下のフェイスボタンがBです。
左スティック押し込みでメニュー、LTでスロット1保存、右スティック押し込みでスロット1読込。
メニューは十字キーで操作し、右のフェイスボタンで決定、下のフェイスボタンで戻ります。
接続・切断は起動後も検出します。実機の接続環境によって認識状況の確認が必要です。

通常速度のステレオ音声出力を追加しました。コア付属のblipperで2,097,152 Hzから32,768 Hzへ変換し、
SDL2で再生します。音声デバイスが使えない場合は画面通知のうえ無音で動作します。

非アクティブ時は一時停止します。通常セーブは10秒ごと、ステート保存時、正常終了時に保存します。
保存先は`%APPDATA%\GBWiz\ROM名.sav`と`ROM名.slot1.state`〜`ROM名.slot5.state`です。
AB入れ替えと選択スロットは同フォルダの`settings.ini`に保存します。
非アクティブ化時にも通常セーブを保存し、メニュー中・非アクティブ中の音声を停止します。
同名ROMは保存先も同じになるため、別ゲームには異なるファイル名を付けてください。
検証時は`GBWiz.exe ROMパス --data-dir 保存フォルダ`で保存先を隔離できます。

## 配布版

開発・検証は`build/Release`で行います。[リリース候補の説明](docs/release-guide.md)と[変更内容](docs/release-notes.md)を用意しています。
`package.ps1 -Version 0.1.2 -SkipBuild -SignedAndroidApk build/GBWiz-0.1.2.apk`は、同階層の`GBWiz_app/0.1.2`へWindows版・署名済みAndroid版・説明書・ライセンス・チェックサムをまとめます。ソースは含めず、既存の版フォルダは上書きしません。
正式なAndroid署名は`sign-release.ps1`でGBWiz専用鍵を指定して行います。従来と同じGBWiz専用鍵を使用します。

## 残りの実装

- 対応7作品の倍速＋等速音声の全場面検証
- 作成ボーナス・MP・人物編集などの作品別対応拡大と、未確認ROM版の検証
- Android実機での操作・音声・Documents保存の検証とUI調整

## 外部コード

`third_party/gambatte`は[libretro/gambatte-libretro](https://github.com/libretro/gambatte-libretro)から取得。
固定コミット: `7809bb68ee96c3e6c74e9d9c4c8bb09aa7c6801d`。
GPL v2。ライセンス本文は`third_party/gambatte/COPYING`に保持しています。
コアはC++14、アプリはC++17でビルドします。
ステート保存時の作業構造体をゼロ初期化し、対象外マッパーの未設定項目にスタック上の値が混入しないようにしています。
GBWiz側の修正: `libgambatte/src/video_libretro.cpp`のRGB32出力で、GBCの5ビット色成分を
8ビットへ展開しています（2026-10-28）。修正前は白でも最大31となり、GBC画面が暗く表示されていました。
`cpu.cpp`・`cpu.h`・`gambatte.cpp`・`gambatte.h`には、元の書き込みとタイミングを保つ任意のCPU書き込み観測APIを追加しています。
ROMは配布物に含めません。

音声・コントローラーには[SDL2 2.32.10](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10)
公式VC開発パッケージを使用。zlibライセンス本文は`third_party/SDL2-2.32.10/LICENSE.txt`。
音声変換のblipperはGambatteに同梱されたMITライセンスのコードをそのまま使用します。

## 検証記録

2026-10-28: Visual Studio 2026 / MSVC 19.51、Release x64ビルド成功。
ローカルの8本（GB外伝1〜3、GBC版Wizardry 1〜3、DQ1&2、DQ3）で
180フレーム・非単色映像・ステート保存復元のスモークチェックに成功。
追加検証: `ctest --test-dir build -C Release --output-on-failure`で、音声変換のサンプル数・音程・
左右チャンネル・リセット、SDL仮想パッドでのNES配列・ショートカット・AB交換・押し続け時の
保存連発防止・切断、ダミー音声デバイスへのキュー投入を確認。
実画面でメニュー表示、スロット2保存・読込、AB設定保存、ゲーム復帰を確認しました。
実物のコントローラー接続、音の聴感、長時間プレイ、通常セーブのゲーム内復帰は未検証です。
GBC色変換の回帰テストで全32,768色のRGB32変換、チャンネル順、色補正時の白・黒を確認します。
ローカルに対応ROMがある場合は`game_lifecycle`でGB/GBC読込、リセット時のSRAM保持、
通常セーブの再読込、不正ROM・不整合セーブが現在のゲームを変更しないことを確認します。
このテストは`build/game-test`内の専用データだけを使用します。
実画面では確認なしのリセット、ROM選択キャンセルによる元のメニューへの復帰、
GB外伝1からGBC版Wizardry 1への切り替えを確認しました。

外伝1〜3とGBC Wizardry 1〜3では全体設定の[MP消費なし](docs/mp-support.md)をON／OFFできます。キャラクター選択は不要です。

## ウィザードリィ共通の自前マップ

外伝I・II・IIIとGBC I・II・IIIに対応。M / RT・ZR / Androidの「マップ」で、迷宮・キャンプ中に呼び出せます。矢印でXY、PageUp / PageDown（LB / RB）で階層を変更し、Aで移動、Bでキャンセルします。移動前にスロット1へ必ず保存します。[詳細](docs/dungeon-map.md)

## ウィザードリィ共通の経験値・お金倍率

外伝I〜III・GBC版I〜IIIの6作品で、×1／×8／×16／×32／×64／×128／×256を独立設定・併用できます。人数配分後の値を倍率化し、通常セーブにも反映します。[仕様と検証](docs/reward-support.md)を参照してください。

## ライセンスと対応ソース

GBWiz独自のコード・文書・独自アセットは[MIT License](LICENSE)です。第三者コードや改変した第三者ファイルには、それぞれのライセンスが適用されます。

GambatteはGPLv2であり、これをリンクしたWindows／Androidアプリの配布ではGPLv2の条件も満たす必要があります。GBWiz独自部分のMITライセンスは維持されます。内訳は[第三者ライセンス](THIRD_PARTY_NOTICES.md)、改変箇所は[Gambatte変更記録](docs/gambatte-changes.md)を参照してください。

0.1.2の対応ソースは[タグv0.1.2](https://github.com/bryful/GBWiz/tree/v0.1.2)から取得できます。[ソースZIP](https://github.com/bryful/GBWiz/archive/refs/tags/v0.1.2.zip)はアプリ配布物と別に取得でき、ビルド手順は本READMEと[Android説明書](android/README.md)に記載しています。

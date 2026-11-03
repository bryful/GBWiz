# GBWiz 0.1.3 検証結果

2026-11-03。

- Windows x64 Releaseのビルド成功。
- SDL仮想コントローラーでX→START、Y→SELECT、XY入れ替え、既存AB入力・方向入力・ショートカットを確認（desktopテスト成功）。
- Androidの変更済みJavaコードのコンパイル成功。
- 実機Xbox／ProコントローラーでのXY操作と、Androidの設定保持・メニュー表示は未確認です。
- 今回は入力設定の変更に必要な確認のみ実施。全ゲーム・全支援機能の再テストは行っていません。

## 既知の制限

- Android初期ROM場所はDocuments/GBWiz/romです。任意フォルダーを選択できます。
- DQ3の性格変更・MP消費なし・全職業選択などは未対応です。
- 自前マップには一部の地形変化が反映されず、直接移動のイベント発火は通常歩行と異なる場合があります。
- ステート読込は人物データも保存時点へ戻します。

リリースAPK自体の実機インストール・起動は未確認です。

## 配布ビルドの確認

- Windows 0.1.3 x64 Release、Android jp.gbwiz／versionCode 9／versionName 0.1.3のreleaseビルド成功。
- Android lintRelease成功（0 errors、11 warnings）。
- APK署名v2／v3検証成功。従来0.1.2と同じGBWiz証明書。
- APKと両ABIのELF LOADセグメントの16KB整列を確認。

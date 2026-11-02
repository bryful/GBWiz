# GBWiz 0.1.2 技術・開発情報

利用者向けの導入・操作説明は配布フォルダー直下のREADME.mdを参照してください。この資料は配布構成、実装、検証と開発上の残件をまとめたものです。

## 配布ビルド

- Windows x64 Release。GBWiz.exe、SDL2.dll、空のRomフォルダーを収録。Windows ZIPは生成しません。
- Android：applicationId jp.gbwiz、versionCode 8、versionName 0.1.2。最小SDK26、arm64-v8a／x86_64、非debuggable。
- Android APKは既存GBWiz専用鍵で署名。0.1.0／0.1.1との更新互換性を維持します。APK v2／v3署名と16KB整列を検証しています。
- ソースは別途配布します。アプリ配布物へソースZIP・ROM・保存データ・秘密鍵・資格情報・開発APK・未署名APKは同梱しません。
- SHA256SUMS.txtは配布ファイルの照合用です。ROM判別・保存名にはハッシュを使用しません。

## 実装上の補足

- 報酬倍率は原作の人数配分後の1人分に適用し、原作の対象判定・加算・表示・保存を使います。
- 外伝は32ビット上限、GBC版Wizardryは12桁BCDの原作上限を維持します。DQ3は職業別経験値上限・所持金999,999を維持し、商人の追加獲得分にも倍率を適用します。
- 設定は再起動・リセット後も維持します。ステート読込でも現在のサポート設定を保持します。報酬表示時間の変化により後続の乱数結果が変わる場合があります。
- 自前マップ移動は原作のマロールを使わず、位置・階層を書き換えて原作の地形読込処理へ戻します。
- MP消費なしは確認済みの原作減算経路だけを抑止し、現在MPの補充・最大MPの変更は行いません。外伝IIIは21段階、GBC版は上位4ビットの現在MPを扱います。
- ROMヘッダーと命令列を照合し、未確認版の個別サポートは無効にします。

## 検証と残件

Windows全103テストと7作品の配布EXEスモークを確認。Pixel 9aの別ID開発版で新しいMP処理とメニューを検証しています。今回の署名済みrelease APK自体の実機インストールは行っていません。

全呪文を画面から選ぶ手動プレイ、全イベント・長時間プレイ、実物コントローラーの網羅確認は残っています。Androidの初期ROM場所はDocuments/GBWiz/romで、開発要件のDocuments/NesDQとの整合は残件です。

詳細は配布直下のVALIDATION.md、BUILD-INFO.txtと、docs内のmp-support.md、dungeon-map.md、reward-support.md、各作品の解析資料を参照してください。

## ライセンスと対応ソース

GBWiz独自部分はMIT、GambatteはGPLv2、SDL2はzlib、blipperはMITです。Gambatteをリンクした配布アプリはGPLv2の条件にも従います。詳細は配布直下のTHIRD-PARTY-NOTICES.mdと各ライセンス本文を参照してください。

0.1.2の対応ソースは https://github.com/bryful/GBWiz/tree/v0.1.2 、別配布のソースZIPは https://github.com/bryful/GBWiz/archive/refs/tags/v0.1.2.zip です。ビルド手順と必要な第三者ソース・ライブラリはソース側に収録しています。

# 0.1.0 リリース作成記録

2026-10-31時点の機能を同階層GBWiz_app/0.1.0へまとめます。rc1〜rc4は保持します。

Windows x64 ReleaseとWindows ZIP、Android署名済みrelease APK、説明書・ライセンス・検証記録・SHA256SUMSを収録します。開発APK、未署名APK、ソースは含めません。ソース配布は別で行います。

秘密鍵はAGENTS.md指定のGBWizと同階層GBWiz_signingへ置きます。GBWiz専用鍵を使用し、フォルダは所有ユーザーとSYSTEMだけにアクセスを限定、パスワードはWindowsアカウントのDPAPIで暗号化します。別PC・別アカウントへの移行前に資格情報を移す必要があります。鍵・資格情報は配布フォルダへコピーしません。

Windows全テスト、配布EXEの7作品起動、Android release・テストAPKのビルドとlint、版番号・ABI・署名・16KBページ整列、配布物の除外対象とチェックサムを確認します。実機の旧開発版は同じアプリIDでも署名が異なるため自動で削除せず、正式署名の実機起動確認には検証専用の別IDを使用します。

外部への公開・アップロードはこのローカル配布物の作成とは別です。

今回の作成中に端末が未接続となったため、正式署名版の実機起動は未確認です。検証専用の別IDでreleaseビルドできることまで確認しています。Windows全73テストと配布EXEの7作品起動、Android署名と16KBページ整列は成功しました。

# 0.1.1 リリース作成記録

2026-11-01の最新実装を同階層GBWiz_app/0.1.1へまとめます。既存0.1.0は保持します。Windows、署名済みAndroid APK、説明書・ライセンス・検証記録・チェックサムを収録し、ソース・開発APK・未署名APK・ROM・保存データ・秘密鍵は収録しません。AndroidはversionCode 7、applicationIdはjp.gbwizを使用します。

0.1.1の作成完了。Windows全99テスト、配布EXEの対象7作品スモーク、Android releaseビルドとlint、最終コードの検証用Debug版での実機14テストが成功しました。APK署名は0.1.0と同じ証明書で、APK／ELFの16KB整列を確認済み。配布内容とWindows ZIPの除外対象、SHA256SUMSを照合しました。今回の署名済みrelease APK自体の実機インストールは行っていません。

# 0.1.2 リリース作成記録

2026-11-02のMP拡張を含むリリースをGBWiz_app/0.1.2へ作成。Windows x64と署名済みAndroid（jp.gbwiz、versionCode 8、両ABI）、説明書、ライセンス、検証記録、SHA256SUMSを収録。既存版は保持。

Windows全103テスト、配布EXEの7作品スモーク、Android releaseビルドとlint、従来証明書の一致、APK／ELFの16KB整列、配布物の除外対象とチェックサムを確認。MP機能はPixel 9aの別ID開発版で検証済み。今回の配布APK自体の実機インストールと外部公開は行っていません。

0.1.2のWindows ZIPは配布対象から除外。今後のpackage.ps1もZIPを生成せず、Windowsフォルダーをそのまま収録します。

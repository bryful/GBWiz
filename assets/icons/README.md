# GBWiz アイコン

現在の採用画像は`gbwiz-icon.png`です。ユーザー提供のWiizアイコンを元に、文字を指定どおり**GBWz**へ変更し、カセットを少し縦長のGB風へ寄せました。アプリ名GBWizは変更していません。

元画像：`../Wiiz/assets/wiiz-icon.png（別プロジェクト）`。元ファイルは変更していません。
編集には組み込みimagegenを使用しました。[実際の編集指示](icon-prompt.md)を記録しています。
Androidの各密度PNGとadaptive icon用の画像は`tools/render-icons.cjs`とsharpで生成し、Windows用ICOは`tools/prepare-icons.py`とPillowで生成します。
Androidではカセットが円形マスクで切れない余白を設けています。

旧候補A（帽子と十字キー）・B（迷宮）・C（携帯機）のSVG／PNGと比較画像は履歴として残しています。現在の書き出し先には使用しません。0.1.0-rc1候補は旧Aのまま保持し、変更後のアイコンは0.1.0-rc2以降へ反映します。

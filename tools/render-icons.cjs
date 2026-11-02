// Export the approved bitmap icon with sharp. Pass a directory containing sharp
// as the first argument, or install sharp locally and omit that argument.
const fs = require('fs');
const path = require('path');
const sharp = require(process.argv[2] ? path.join(process.argv[2], 'sharp') : 'sharp');
const root = path.resolve(__dirname, '..');
(async () => {
  const source = path.join(root, 'assets/icons/gbwiz-icon.png');
  for (const [density, size] of [['mdpi',48],['hdpi',72],['xhdpi',96],['xxhdpi',144],['xxxhdpi',192]]) {
    const directory = path.join(root, 'android/app/src/main/res', `mipmap-${density}`);
    fs.mkdirSync(directory, {recursive:true});
    await sharp(source).resize(size,size).png().toFile(path.join(directory,'ic_launcher.png'));
  }
  const art = path.join(root, 'android/app/src/main/res/drawable-nodpi');
  fs.mkdirSync(art, {recursive:true});
  await sharp(source).resize(512,512).png().toFile(path.join(art, 'ic_launcher_art.png'));
})().catch(error => { console.error(error); process.exit(1); });

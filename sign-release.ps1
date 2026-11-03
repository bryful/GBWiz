param(
    [string]$InputApk = 'android\app\build\outputs\apk\release\app-release-unsigned.apk',
    [string]$OutputApk = 'build\GBWiz-0.1.3.apk',
    [string]$Keystore = '..\GBWiz_signing\GBWiz-release.jks',
    [string]$KeyAlias = 'gbwiz',
    [string]$SdkPath = $env:ANDROID_HOME
)
$ErrorActionPreference = 'Stop'
if (!$SdkPath) { $SdkPath = Join-Path $env:LOCALAPPDATA 'Android\Sdk' }
if (!(Test-Path -LiteralPath $Keystore)) { throw 'GBWiz release keystore is not prepared. Use a GBWiz-specific key in the AGENTS.md signing directory.' }
if (!(Test-Path -LiteralPath $InputApk)) { throw 'Unsigned release APK not found. Build assembleRelease first.' }
if (Test-Path -LiteralPath $OutputApk) { throw 'Output APK already exists; choose another output path.' }
$credentials=Join-Path (Split-Path -Parent $Keystore) 'credentials.dpapi.xml'
$previousStore=$env:GBWIZ_STORE_PASSWORD;$previousKey=$env:GBWIZ_KEY_PASSWORD
$aligned=Join-Path ([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($OutputApk))) ('aligned-'+[guid]::NewGuid().ToString('N')+'.apk')
try{
 if((!$env:GBWIZ_STORE_PASSWORD -or !$env:GBWIZ_KEY_PASSWORD) -and (Test-Path -LiteralPath $credentials)){
  $stored=Import-Clixml -LiteralPath $credentials
  $env:GBWIZ_STORE_PASSWORD=[Net.NetworkCredential]::new('',$stored.StorePassword).Password
  $env:GBWIZ_KEY_PASSWORD=[Net.NetworkCredential]::new('',$stored.KeyPassword).Password
 }
 if(!$env:GBWIZ_STORE_PASSWORD -or !$env:GBWIZ_KEY_PASSWORD){throw 'GBWiz signing credentials are unavailable.'}
 $signer=Join-Path $SdkPath 'build-tools\36.0.0\apksigner.bat'
 $aligner=Join-Path $SdkPath 'build-tools\36.0.0\zipalign.exe'
 & $aligner -P 16 -f 4 $InputApk $aligned
 if($LASTEXITCODE -ne 0){throw 'APK alignment failed.'}
 & $signer sign --ks $Keystore --ks-key-alias $KeyAlias --ks-pass env:GBWIZ_STORE_PASSWORD --key-pass env:GBWIZ_KEY_PASSWORD --out $OutputApk $aligned
 if($LASTEXITCODE -ne 0){throw 'APK signing failed.'}
 & $signer verify --verbose --print-certs $OutputApk
 if($LASTEXITCODE -ne 0){throw 'APK signature verification failed.'}
 & $aligner -c -P 16 4 $OutputApk
 if($LASTEXITCODE -ne 0){throw 'Signed APK alignment verification failed.'}
}finally{
 $env:GBWIZ_STORE_PASSWORD=$previousStore;$env:GBWIZ_KEY_PASSWORD=$previousKey
 if(Test-Path -LiteralPath $aligned){Remove-Item -LiteralPath $aligned -Force}
}

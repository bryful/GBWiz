param([string]$Version='0.1.3',[switch]$SkipBuild,[switch]$IncludeAndroidRelease,[switch]$IncludeAndroidDebug,[string]$SignedAndroidApk)
$ErrorActionPreference='Stop'
if($Version -notmatch '^\d+\.\d+\.\d+(?:-[a-zA-Z0-9.-]+)?$'){throw 'Invalid release version.'}
if(!$SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1')}
$destination=Join-Path ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\GBWiz_app'))) $Version
if(Test-Path -LiteralPath $destination){throw "Release folder already exists; preserve it and choose another version: $destination"}
$required=@('build\Release\GBWiz.exe','build\Release\SDL2.dll','docs\release-notes.md','docs\release-guide.md','docs\release-validation.md','LICENSE','THIRD_PARTY_NOTICES.md')
if($IncludeAndroidRelease){$required+='android\app\build\outputs\apk\release\app-release-unsigned.apk'}
foreach($file in $required){if(!(Test-Path -LiteralPath (Join-Path $PSScriptRoot $file))){throw "Missing release input: $file"}}
if($IncludeAndroidDebug){$debugApk=Join-Path $PSScriptRoot 'android\app\build\outputs\apk\debug\app-debug.apk';if(!(Test-Path -LiteralPath $debugApk)){throw 'Missing Android debug APK.'}}
$versionInfo=[Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $PSScriptRoot 'build\Release\GBWiz.exe'))
if($versionInfo.FileVersion -ne $Version){throw 'Windows binary version does not match package version.'}
if($SignedAndroidApk -and !(Test-Path -LiteralPath $SignedAndroidApk)){throw 'Signed Android APK missing.'}
$windows=Join-Path $destination 'Windows'
New-Item -ItemType Directory -Path (Join-Path $windows 'Rom') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build\Release\GBWiz.exe'),(Join-Path $PSScriptRoot 'build\Release\SDL2.dll') -Destination $windows
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs\release-guide.md') -Destination (Join-Path $windows 'README.md')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'LICENSE') -Destination (Join-Path $destination 'MIT-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'THIRD_PARTY_NOTICES.md') -Destination (Join-Path $destination 'THIRD-PARTY-NOTICES.md')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'third_party\gambatte\COPYING') -Destination (Join-Path $destination 'COPYING.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'third_party\SDL2-2.32.10\LICENSE.txt') -Destination (Join-Path $destination 'SDL2-LICENSE.txt')
$blipper=Get-Content -Raw -Encoding UTF8 (Join-Path $PSScriptRoot 'third_party\gambatte\libgambatte\libretro\blipper.c')
$blipper.Substring(0,$blipper.IndexOf('*/')+2) | Set-Content -Encoding UTF8 (Join-Path $destination 'blipper-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs') -Destination $destination -Recurse
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs\release-notes.md') -Destination (Join-Path $destination 'RELEASE-NOTES.md')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs\release-guide.md') -Destination (Join-Path $destination 'README.md')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs\release-validation.md') -Destination (Join-Path $destination 'VALIDATION.md')
$buildInfo=Join-Path $PSScriptRoot 'build\release-build-info.txt'
if(Test-Path -LiteralPath $buildInfo){Copy-Item -LiteralPath $buildInfo -Destination (Join-Path $destination 'BUILD-INFO.txt')}
if($IncludeAndroidRelease){
 $androidOutput=Join-Path $destination 'Android'
 New-Item -ItemType Directory -Path $androidOutput | Out-Null
 Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'android\app\build\outputs\apk\release\app-release-unsigned.apk') -Destination (Join-Path $androidOutput "GBWiz-$Version-unsigned.apk")
}
if($IncludeAndroidDebug){
 $androidOutput=Join-Path $destination 'Android'
 New-Item -ItemType Directory -Path $androidOutput -Force | Out-Null
 Copy-Item -LiteralPath $debugApk -Destination (Join-Path $androidOutput "GBWiz-$Version-dev.apk")
}
if($SignedAndroidApk){
 $androidOutput=Join-Path $destination 'Android'
 New-Item -ItemType Directory -Path $androidOutput -Force | Out-Null
 Copy-Item -LiteralPath $SignedAndroidApk -Destination (Join-Path $androidOutput "GBWiz-$Version.apk")
}
# Application packages exclude sources; corresponding source is distributed separately.
$hashLines=Get-ChildItem -LiteralPath $destination -File -Recurse | ForEach-Object {
 $relative=$_.FullName.Substring($destination.Length+1).Replace('\','/')
 '{0}  {1}' -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(),$relative
}
$hashLines | Set-Content -Encoding UTF8 (Join-Path $destination 'SHA256SUMS.txt')
Write-Output "Release prepared: $destination"

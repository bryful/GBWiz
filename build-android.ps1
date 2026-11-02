param([switch]$Offline,[string[]]$Tasks=@('assembleDebug'))
$ErrorActionPreference = 'Stop'
$sdkPath = $env:ANDROID_HOME
if (!$sdkPath) { $sdkPath = Join-Path $env:LOCALAPPDATA 'Android\Sdk' }
$env:ANDROID_HOME = $sdkPath
if (!$env:JAVA_HOME) {
    $jdkCandidates = @('C:\Program Files\Android\openjdk\jdk-21.0.8','C:\Program Files\Android\Android Studio1\jbr','C:\Program Files\Android\Android Studio\jbr')
    $env:JAVA_HOME = $jdkCandidates | Where-Object { Test-Path -LiteralPath (Join-Path $_ 'bin\java.exe') } | Select-Object -First 1
}
if (!$env:JAVA_HOME) { throw 'Set JAVA_HOME to JDK 17 or 21.' }
$projectPath = Join-Path $PSScriptRoot 'android'
$gradleCommand = Join-Path $projectPath 'gradlew.bat'
if (!(Test-Path -LiteralPath $gradleCommand)) {
    $gradleCommand = Get-ChildItem -LiteralPath (Join-Path $env:USERPROFILE '.gradle\wrapper\dists\gradle-8.12-bin') -Recurse -Filter gradle.bat | Select-Object -First 1 -ExpandProperty FullName
}
if (!$gradleCommand) { throw 'Gradle 8.12 not found.' }
$buildArguments = @('-p',$projectPath,'--no-daemon','--console=plain') + $Tasks
if ($Offline) { $buildArguments += '--offline' }
& $gradleCommand @buildArguments
if ($LASTEXITCODE -ne 0) { throw 'Android build failed.' }

param([string]$SigningDirectory=(Join-Path $PSScriptRoot '..\GBWiz_signing'))
$ErrorActionPreference='Stop'
$signingPath=[IO.Path]::GetFullPath($SigningDirectory)
$expectedPath=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\GBWiz_signing'))
if($signingPath -ne $expectedPath){throw 'Use the GBWiz_signing directory specified by AGENTS.md.'}
New-Item -ItemType Directory -Path $signingPath -Force | Out-Null
$owner=[Security.Principal.WindowsIdentity]::GetCurrent().User
$acl=[Security.AccessControl.DirectorySecurity]::new()
$acl.SetOwner($owner);$acl.SetAccessRuleProtection($true,$false)
foreach($sid in @($owner,[Security.Principal.SecurityIdentifier]::new('S-1-5-18'))){
 $rule=[Security.AccessControl.FileSystemAccessRule]::new($sid,'FullControl','ContainerInherit,ObjectInherit','None','Allow')
 $acl.AddAccessRule($rule)
}
Set-Acl -LiteralPath $signingPath -AclObject $acl
$keystore=Join-Path $signingPath 'GBWiz-release.jks'
$credentials=Join-Path $signingPath 'credentials.dpapi.xml'
if(Test-Path -LiteralPath $keystore){throw 'Signing key already exists; it has not been replaced.'}
if(Test-Path -LiteralPath $credentials){
 $stored=Import-Clixml -LiteralPath $credentials
}else{
 $bytes=New-Object byte[] 48;$rng=[Security.Cryptography.RandomNumberGenerator]::Create()
 try{$rng.GetBytes($bytes)}finally{$rng.Dispose()}
 $secret=[Convert]::ToBase64String($bytes)
 $secure=ConvertTo-SecureString $secret -AsPlainText -Force
 $stored=[pscustomobject]@{Alias='gbwiz';StorePassword=$secure;KeyPassword=$secure}
 $stored | Export-Clixml -LiteralPath $credentials -Encoding UTF8
 $secret=$null;[Array]::Clear($bytes,0,$bytes.Length)
}
$jdk=$env:JAVA_HOME
if(!$jdk){$jdk='C:\Program Files\Android\openjdk\jdk-21.0.8'}
$keytool=Join-Path $jdk 'bin\keytool.exe'
try{
 $env:GBWIZ_STORE_PASSWORD=[Net.NetworkCredential]::new('',$stored.StorePassword).Password
 $env:GBWIZ_KEY_PASSWORD=[Net.NetworkCredential]::new('',$stored.KeyPassword).Password
 & $keytool -genkeypair -keystore $keystore -storetype JKS -alias gbwiz -keyalg RSA -keysize 3072 -sigalg SHA256withRSA -validity 10000 -dname 'CN=GBWiz, O=GBWiz, C=JP' -storepass:env GBWIZ_STORE_PASSWORD -keypass:env GBWIZ_KEY_PASSWORD -noprompt
 if($LASTEXITCODE -ne 0){throw 'GBWiz key generation failed.'}
}finally{Remove-Item Env:GBWIZ_STORE_PASSWORD,Env:GBWIZ_KEY_PASSWORD -ErrorAction SilentlyContinue}
$privateNote=@'
GBWiz Android release signing

GBWiz-release.jks is the dedicated GBWiz signing key. Do not publish it.
credentials.dpapi.xml protects its passwords with Windows DPAPI for this
Windows account on this computer. sign-release.ps1 loads it automatically.
Neither file belongs in GBWiz_app or source distribution.

Back up this directory privately. Before moving to a different Windows account
or computer, export the passwords from DPAPI under the current account into a
trusted password manager. A copy of this directory alone cannot decrypt DPAPI
credentials on a different Windows account/computer. Never replace the key for
updates to an existing released application.
'@
$privateNote | Set-Content -LiteralPath (Join-Path $signingPath 'README-PRIVATE.txt') -Encoding UTF8
Write-Output "GBWiz signing key prepared in $signingPath (passwords not displayed)."

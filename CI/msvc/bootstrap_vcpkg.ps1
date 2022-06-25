CD ../..

if(Test-Path './build') {
	Write-Host 'Build directory already exists'
}
else{
	$buildDir = New-Item 'build' -ItemType Directory
	Write-Host "Creating build directory $($buildDir.FullName)"
}

CD './build'

$vcpkgDownloaded = Test-Path './vcpkg.7z'
if($vcpkgDownloaded -eq $False){
	Write-Host 'Downloading VCPKG'
	Invoke-WebRequest -Uri "https://github.com/vcmi/vcmi-deps-windows/releases/download/v1.4/vcpkg-export-x64-windows-v140.7z" -OutFile "vcpkg.7z"
}

$vcpkgDownloaded = Test-Path './vcpkg'
if($vcpkgDownloaded -eq $False){
	Write-Host 'Unpacking VCPKG'
	&'C:\Program Files\7-Zip\7z.exe' x vcpkg.7z
}

$debugCopied = Test-Path './vcpkg/installed/x64-windows/debug/bin'
if($debugCopied -eq $False){
	New-Item './vcpkg/installed/x64-windows/debug/bin' -ItemType Directory
	Copy-Item -Path './vcpkg/installed/x64-windows/bin/*' -Destination './vcpkg/installed/x64-windows/debug/bin' -Recurse
}

if(Test-Path './CMakeCache.txt'){
	Write-Host 'CmakeCache.txt already exists. Do you want to remove it (y/n)?'
	$resp = Read-Host
	if($resp -eq 'y'){
		Remove-Item './CMakeCache.txt'
	}
}

Write-Host 'Configuring cmake'
cmake .. -DENABLE_TEST=0 '-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake' -G 'Visual Studio 16 2019' -A x64

CD ../CI/msvc
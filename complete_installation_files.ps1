# Source: "ipc-fullscreen-viewer.exe"; DestDir: "{app}"; Flags: ignoreversion

# [Registry]
# Root: HKA; Subkey: "Software\Classes\ipc-fullscreen-viewer.exe"; ValueType: string; ValueName: ""; ValueData: "ipc-fullscreen-viewer"; Flags: uninsdeletekey
# Root: HKA; Subkey: "Software\Classes\ipc-fullscreen-viewer.exe\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\ipc-fullscreen-viewer.exe,0"
# Root: HKA; Subkey: "Software\Classes\ipc-fullscreen-viewer.exe\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\ipc-fullscreen-viewer.exe"" ""%1"""

# [Icons]
# Name: "{autoprograms}\{#MyAppName}\ipc-fullscreen-viewer"; Filename: "{app}\ipc-fullscreen-viewer.exe"
# Name: "{autodesktop}\ipc-fullscreen-viewer"; Filename: "{app}\ipc-fullscreen-viewer.exe"; Tasks: desktopicon

$global:files = ""
$global:registry = ""
$global:icons = ""

function addExecutable {
    Param ([string]$EXEName, [string]$AppName, [string]$InPath, [string]$OutPath)

    $inputFile = $InPath
    If ($inputFile -ne "") {
        $inputFile = "$inputFile\"
    }
    $inputFile = "$inputFile$EXEName"

    $outputFile = $OutPath
    If ($outputFile -ne "") {
        $outputFile = "$outputFile\"
    }
    $outputFile = "$outputFile$EXEName"
    

    Write-Output "$EXEName ($AppName):"
    Write-Output "    $inputFile -> $outputFile"

    If ($OutPath -ne "") {
        $OutPath = "\$OutPath"
    }

    $global:files += "Source: ""$inputFile""; DestDir: ""{app}$OutPath""; Flags: ignoreversion`n"

    $global:registry += "Root: HKA; Subkey: ""Software\Classes\$EXEName""; ValueType: string; ValueName: """"; ValueData: ""$AppName""; Flags: uninsdeletekey`n"
    $global:registry += "Root: HKA; Subkey: ""Software\Classes\$EXEName\DefaultIcon""; ValueType: string; ValueName: """"; ValueData: ""{app}$OutPath\$EXEName,0""`n"
    $global:registry += "Root: HKA; Subkey: ""Software\Classes\$EXEName\shell\open\command""; ValueType: string; ValueName: """"; ValueData: """"""{app}$OutPath\$EXEName"""" """"%1""""""`n"


    $global:icons += "Name: ""{autoprograms}\{#MyAppName}\$AppName""; Filename: ""{app}$OutPath\$EXEName""`n"
    $global:icons += "Name: ""{autodesktop}\$AppName""; Filename: ""{app}$OutPath\$EXEName""; Tasks: desktopicon`n"

}

function addFile {
    Param ([string]$Filename, [string]$InPath, [string]$OutPath)

    $inputFile = $InPath
    If ($inputFile -ne "") {
        $inputFile = "$inputFile\"
    }
    $inputFile = "$inputFile$Filename"

    $outputFile = $OutPath
    If ($outputFile -ne "") {
        $outputFile = "$outputFile\"
    }
    $outputFile = "$outputFile$Filename"

    Write-Output "${Filename}:"
    Write-Output "    $inputFile -> $outputFile"

    If ($OutPath -ne "") {
        $OutPath = "\$OutPath"
    }

    $global:files += "Source: ""$inputFile""; DestDir: ""{app}$OutPath""; Flags: ignoreversion`n"
}

addExecutable -EXEName "ipc-fullscreen-viewer.exe" -AppName "Fullscreen Viewer" -InPath "" -OutPath ""
addFile -Filename "Roboto-Regular-80.basof2" -InPath "resources" -OutPath "resources"

addExecutable -EXEName "debug-console.exe" -AppName "Debug Console" -InPath "" -OutPath ""

addExecutable -EXEName "network_to_ipc_ip.bat" -AppName "Ip Device Connect" -InPath "network-to-ipc" -OutPath "network-to-ipc"
addFile -Filename "avcodec-58.dll" -InPath "network-to-ipc" -OutPath "network-to-ipc"
addFile -Filename "avformat-58.dll" -InPath "network-to-ipc" -OutPath "network-to-ipc"
addFile -Filename "avutil-56.dll" -InPath "network-to-ipc" -OutPath "network-to-ipc"
addFile -Filename "background_no_connection.png" -InPath "network-to-ipc" -OutPath "network-to-ipc"
addFile -Filename "network-to-ipc.exe" -InPath "network-to-ipc" -OutPath "network-to-ipc"
addFile -Filename "swresample-3.dll" -InPath "network-to-ipc" -OutPath "network-to-ipc"
addFile -Filename "swscale-5.dll" -InPath "network-to-ipc" -OutPath "network-to-ipc"
addFile -Filename "ffmpeg_license_LGPLv2_1.txt" -InPath "network-to-ipc" -OutPath "network-to-ipc"
addFile -Filename "ffmpeg_license_LGPLv3.txt" -InPath "network-to-ipc" -OutPath "network-to-ipc"


Write-Output "[Files]"
Write-Output $global:files
Write-Output "[Registry]"
Write-Output $global:registry
Write-Output "[Icons]"
Write-Output $global:icons

$final_str = "`n"+${global:files} + "`n[Registry]`n"+${global:registry} + "`n[Icons]`n"+${global:icons}

Copy-Item ".\inno_setup_install_generator_source.iss" -Destination ".\inno_setup_install_generator.iss"
Add-Content -Path ".\inno_setup_install_generator.iss" -Value $final_str

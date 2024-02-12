#at top of script
if (!
    #current role
    (New-Object Security.Principal.WindowsPrincipal(
        [Security.Principal.WindowsIdentity]::GetCurrent()
    #is admin?
    )).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator
    )
) {
    #elevate script and exit current non-elevated runtime
    $script_path = """"+$MyInvocation.MyCommand.Source+""""
    Start-Process `
        -FilePath 'powershell' `
        -ArgumentList (
            #flatten to single array
            '-File', $script_path, $args `
            | %{ $_ }
        ) `
        -Verb RunAs
    exit
}

$scriptpath = $MyInvocation.MyCommand.Path
$dir = Split-Path $scriptpath
Set-Location -Path $dir

Invoke-Expression -Command "regsvr32 ""ipc-dshow-source.dll"""


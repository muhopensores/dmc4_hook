# this script assumes gettext binaries present in your PATH variable
# and binary_to_compressed_c from dear_imgui also
# those can be grabbed here: https://github.com/vslavik/gettext-tools-windows/releases/tag/v0.22.5
# then add bin folder in advanced settings-> enviroment variables

New-Item -ItemType Directory -Force -Path .\i18n\po\en
New-Item -ItemType Directory -Force -Path .\i18n\mo
New-Item -Path .\i18n\po\en\ -Force -Name dmc4hook.po

# (re)create new .po file
Write-Host "Generating new English .po file"
Get-ChildItem -Path .\src\ -Filter *.cpp -Recurse -File | ForEach-Object {
    xgettext --default-domain=dmc4hook --output=.\i18n\po\en\dmc4hook.po --join-existing --from-code=utf-8 --c++ --keyword=_ --keyword=__ --width=80 $_.FullName
}
$content = Get-Content -Path .\i18n\po\en\dmc4hook.po
$econtent = $content -replace 'charset=CHARSET', 'charset=UTF-8'
Set-Content -Path .\i18n\po\en\dmc4hook.po -Value $econtent

# update existing po files
Get-ChildItem -Path .\i18n\po\ -Filter *.po -Recurse -File | ForEach-Object {
    Write-Host "Updating translation for {$_}"
    msgmerge $_.FullName .\i18n\po\en\dmc4hook.po --update --no-fuzzy-matching
}

# compile into mo files
Get-ChildItem -Path .\i18n\po\ -Filter *.po -Recurse -File | ForEach-Object {
    $dir = $_.Directory.FullName
    $out_filename = $dir.Substring($dir.Length - 2) + "_" + $_.BaseName + ".mo"
    Write-Host "Compiling {$_} into .mo file {$out_filename}"
    msgfmt --output-file=.\i18n\mo\$out_filename $_.FullName
}

# compress to cpp files
Get-ChildItem -Path .\i18n\mo\ -Filter *.mo -Recurse -File | ForEach-Object {
    $out_filename = ".\src\i18n\" + $_.BaseName + ".hpp"
    Write-Host "Compressing {$_} into .cpp file {$out_filename}"
    binary_to_compressed_c $_.FullName $_.BaseName > $out_filename
}

Read-Host -Prompt "Here's some time to read errors"
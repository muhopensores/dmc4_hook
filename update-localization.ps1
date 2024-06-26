# this script assumes gettext binaries present in your PATH variable
# and binary_to_compressed_c from dear_imgui also
# those can be grabbed here: https://github.com/vslavik/gettext-tools-windows/releases/tag/v0.22.5
# then add bin folder in advanced settings-> enviroment variables

New-Item -ItemType Directory -Force -Path .\i18n\po\en
New-Item -Path .\i18n\po\en\ -Name dmc4hook.po

Get-ChildItem -Path .\src\ -Filter *.cpp -Recurse -File | ForEach-Object {
    xgettext --default-domain=dmc4hook --output=.\i18n\po\en\dmc4hook.po --join-existing --from-code=utf-8 --c++ --keyword=_ --width=80 $_.FullName
    $content = Get-Content -Path .\i18n\po\en\dmc4hook.po
    $econtent = $content -replace 'charset=CHARSET', 'charset=UTF-8'
    Set-Content -Path .\i18n\po\en\dmc4hook.po -Value $econtent
}

Get-ChildItem -Path .\i18n\po\ -Filter *.po -Recurse -File | ForEach-Object {
    $dir = $_.Directory.FullName
    $out_filename = $dir.Substring($dir.Length - 2) + "_" + $_.BaseName + ".mo"
    msgfmt --output-file=.\i18n\mo\$out_filename $_.FullName
}

Get-ChildItem -Path .\i18n\mo\ -Filter *.mo -Recurse -File | ForEach-Object {
    $out_filename = ".\src\i18n\" + $_.BaseName + ".hpp"
    binary_to_compressed_c $_.FullName $_.BaseName > $out_filename
}
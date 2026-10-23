param([string]$AnnotationDirectory = 'data/emoji')
$ErrorActionPreference = 'Stop'
$keywords = @{}
$names = @{}
foreach ($languageCode in @('es','it','fr','de')) {
foreach ($filename in @("$languageCode.xml","$languageCode-derived.xml")) {
    [xml]$xml = Get-Content (Join-Path $AnnotationDirectory $filename) -Raw -Encoding utf8
    foreach ($entry in $xml.ldml.annotations.annotation) {
        $key = ([string]$entry.cp).Replace([string][char]0xfe0f, '')
        $keywords[$key] = ($keywords[$key] + ' ' + $entry.InnerText).Trim()
        if($entry.type -eq 'tts'){
            if(!$names.ContainsKey($key)){$names[$key]=@{}}
            $names[$key][$languageCode]=$entry.InnerText
        }
    }
}
}
$group = ''
$entries = foreach ($line in Get-Content data/emoji/emoji-test.txt -Encoding utf8) {
    if ($line -match '^# group: (.+)$') { $group = $Matches[1] }
    if ($line -match '^([0-9A-F ]+)\s*;\s*(fully-qualified|component)\s*#\s*\S+\s+E[0-9.]+\s+(.+)$') {
        $symbol = (($Matches[1].Trim() -split '\s+' | ForEach-Object { [char]::ConvertFromUtf32([Convert]::ToInt32($_,16)) }) -join '')
        $name = $Matches[3]
        $key=$symbol.Replace([string][char]0xfe0f,'')
        $localized=@{en=$name}
        if($names.ContainsKey($key)){foreach($code in $names[$key].Keys){$localized[$code]=$names[$key][$code]}}
        [ordered]@{emoji=$symbol;group=$group;name=$name;names=$localized;keywords=$keywords[$key]}
    }
}
if ($entries.Count -lt 3900) { throw 'Incomplete Unicode catalogue' }
$entries | ConvertTo-Json -Depth 5 -Compress | Set-Content data/emoji/catalog.json -Encoding utf8
Write-Output "Generated $($entries.Count) Unicode entries"


powershell -Command "Get-ChildItem -path . -Recurse -Include *.vmt | Foreach-Object { (gc $_) -replace '\"SDK_LightmappedGeneric\"', '\"LightmappedGeneric\"' | Out-File $_" }
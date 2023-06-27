Invoke-WebRequest -Method "DELETE" -Uri "$Env:LAB_BOARD_URL/files"

$FileNames = Get-ChildItem -Recurse -File -Name -Path "$PWD\dist\client" 
foreach ($FileName in $FileNames) {
  $FileStream = [System.IO.FileStream]::new("$PWD\dist\client\$FileName", [System.IO.FileMode]::Open)
  $FileHeader = [System.Net.Http.Headers.ContentDispositionHeaderValue]::new("form-data")
  $FileHeader.Name = "file"
  $FileHeader.FileName = $FileName.Replace("\", "/")
  $FileContent = [System.Net.Http.StreamContent]::new($FileStream)
  $FileContent.Headers.ContentDisposition = $FileHeader
  
  $MultipartContent = [System.Net.Http.MultipartFormDataContent]::new()
  $MultipartContent.Add($FileContent)
  
  Invoke-WebRequest -Body $MultipartContent -Method "POST" -Uri "$Env:LAB_BOARD_URL/upload"

  $FileStream.Dispose()
  $FileContent.Dispose()
  $MultipartContent.Dispose()
}

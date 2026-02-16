param(
    [string]$Port = "COM3",
    [int]$Baud = 9600
)

$sp = New-Object System.IO.Ports.SerialPort $Port, $Baud, 'None', 8, 'One'
$sp.Open()
Write-Host "Listening on $Port at $Baud 8N1..."

while ($true) {
    if ($sp.BytesToRead -gt 0) {
        $char = [char]$sp.ReadChar()
        [Console]::Write($char)  # prints immediately
    } else {
        Start-Sleep -Milliseconds 10
    }
}

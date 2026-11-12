# BLTZCRFT - Logo fuer Titel- und Ladebild aufbereiten
#
# Quelle ist daten/BLTZCRFT-Logo.png (gross, freigestellt mit Alphakanal),
# Ziel daten/logo.png, das das Spiel laedt (laden.bb).
#
# Warum vorab und nicht im Spiel: LoadImage behaelt den Alphakanal der PNG,
# ResizeImage/ScaleImage verwerfen ihn aber wie im Original - der weiche
# Schein um das Logo wuerde zu einem dunklen Kasten. Darum wird hier mit
# bikubischer Filterung verkleinert und das Alpha bleibt erhalten.
#
# Ausserdem maskiert LoadImage reines Schwarz (0,0,0) wie im Original als
# durchsichtig. Sichtbare Pixel dieser Farbe werden darum zu (1,1,1) - sonst
# haetten die dunklen Konturen Loecher.
#
# Aufruf aus dem BLTZCRFT-Ordner:
#   powershell -ExecutionPolicy Bypass -File werkzeug\logo.ps1 [-Breite 720]

param([int]$Breite = 720)

Add-Type -AssemblyName System.Drawing
$ordner = Split-Path -Parent $PSScriptRoot
$quelle = Join-Path $ordner "daten\BLTZCRFT-Logo.png"
$ziel = Join-Path $ordner "daten\logo.png"

$src = [System.Drawing.Image]::FromFile($quelle)
$hoehe = [int][Math]::Round($src.Height * $Breite / $src.Width)
$bild = New-Object System.Drawing.Bitmap $Breite, $hoehe, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bild)
$g.InterpolationMode = 'HighQualityBicubic'
$g.PixelOffsetMode = 'HighQuality'
$g.CompositingMode = 'SourceCopy'
$g.DrawImage($src, 0, 0, $Breite, $hoehe)
$g.Dispose()
$src.Dispose()

# reines Schwarz, das sichtbar ist, um eins anheben (direkt im Speicher)
$rect = New-Object System.Drawing.Rectangle 0, 0, $Breite, $hoehe
$daten = $bild.LockBits($rect, 'ReadWrite', ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb))
$bytes = New-Object byte[] ($daten.Stride * $hoehe)
[System.Runtime.InteropServices.Marshal]::Copy($daten.Scan0, $bytes, 0, $bytes.Length)
$geaendert = 0
for ($i = 0; $i -lt $bytes.Length; $i += 4) {
    # BGRA
    if ($bytes[$i + 3] -gt 0 -and $bytes[$i] -eq 0 -and $bytes[$i + 1] -eq 0 -and $bytes[$i + 2] -eq 0) {
        $bytes[$i] = 1; $bytes[$i + 1] = 1; $bytes[$i + 2] = 1
        $geaendert++
    }
}
[System.Runtime.InteropServices.Marshal]::Copy($bytes, 0, $daten.Scan0, $bytes.Length)
$bild.UnlockBits($daten)

$bild.Save($ziel, [System.Drawing.Imaging.ImageFormat]::Png)
$bild.Dispose()
"daten\logo.png: $Breite x $hoehe, $geaendert schwarze Pixel angehoben"

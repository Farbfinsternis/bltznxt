# Hilfe fuer astra.py: Farb- und Leuchttextur einer Waffe auf Spielgroesse
# bringen und zu einer Farbtextur verschmelzen.
#
# Der Blitz3D-Renderer kennt keine Leuchttextur (glTF emissiveTexture); die
# Leuchtflaechen (hier der glimmende Kern) muessen deshalb in der Farbtextur
# selbst hell sein. Die Leuchtfarbe wird auf die Farbe addiert.
#
#   textur.ps1 farbe.png leuchten.png groesse ausgabe.jpg
param([string]$farbe, [string]$leuchten, [int]$groesse, [string]$aus)

Add-Type -AssemblyName System.Drawing

function Skaliert([string]$pfad, [int]$n) {
    $i = [System.Drawing.Image]::FromFile($pfad)
    $b = New-Object System.Drawing.Bitmap $n, $n, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($b)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.DrawImage($i, 0, 0, $n, $n)
    $g.Dispose(); $i.Dispose()
    return $b
}

function Punkte($b) {
    $r = New-Object System.Drawing.Rectangle 0, 0, $b.Width, $b.Height
    $d = $b.LockBits($r, [System.Drawing.Imaging.ImageLockMode]::ReadWrite, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bytes = New-Object byte[] ($d.Stride * $d.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($d.Scan0, $bytes, 0, $bytes.Length)
    return @{ bits = $d; bytes = $bytes }
}

$f = Skaliert $farbe $groesse
$l = Skaliert $leuchten $groesse
$pf = Punkte $f
$pl = Punkte $l
Add-Type -TypeDefinition @"
public static class Verschmelzen {
    public static void Addiere(byte[] a, byte[] e) {
        for (int i = 0; i < a.Length; i += 4) {
            for (int k = 0; k < 3; k++) {
                int v = a[i + k] + e[i + k];
                a[i + k] = (byte)(v > 255 ? 255 : v);
            }
            a[i + 3] = 255;
        }
    }
}
"@
$a = $pf.bytes
[Verschmelzen]::Addiere($a, $pl.bytes)
[System.Runtime.InteropServices.Marshal]::Copy($a, 0, $pf.bits.Scan0, $a.Length)
$f.UnlockBits($pf.bits); $l.UnlockBits($pl.bits)
# als JPEG (Qualitaet 90): ein Zehntel der PNG
$o = $f.Clone((New-Object System.Drawing.Rectangle 0, 0, $groesse, $groesse), [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
$jpeg = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$q = New-Object System.Drawing.Imaging.EncoderParameters 1
$q.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter ([System.Drawing.Imaging.Encoder]::Quality), ([long]90)
$o.Save($aus, $jpeg, $q)
$o.Dispose(); $f.Dispose(); $l.Dispose()

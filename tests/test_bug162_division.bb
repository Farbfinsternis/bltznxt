; BUG-162 - eine Ganzzahl-Variable durch eine konstante positive Zweierpotenz
; (1<<k, k = 0..31, auch $80000000) rundet im Original ab wie "sar":
; -33 / 16 ist -3. Mit konstantem Zaehler (gefaltet) oder variablem Teiler
; wird abgeschnitten. Am Original gemessen (build/div20260918).

Const K = 16
Const M = $80000000
Local i% = -33
Local p% = 33
Local d% = 16
Print i / 16
Print i / K
Print i / (2 * 8)
Print i / Int(16.0)
Print i / Abs(-4)
Print i / 1
Print i / M
Print p / M
Print i / -16
Print i / d
Print -33 / 16
Print K / 16
Print (i + 0) / 16
Print i / 2.0
Print i / 3
Print i / 1073741824
Local e% = -2147483647 - 1
Print e / 2
Print e / 1
Print p / 16
Print i * 16
Print -1 / 2
Local kk% = -1
Print kk / 2
Print kk / 1
i = i / 4
Print i
Local f# = -33.0
Print f / 16
Print Int(f) / 16
Print i / K / 2

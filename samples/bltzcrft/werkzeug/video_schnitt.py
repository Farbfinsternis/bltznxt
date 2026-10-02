# BLTZCRFT - Werbevideo zusammensetzen (Blender, ohne Oberflaeche).
#
# Nimmt die Einzelbilder von werkzeug/video.bb (f0000.bmp ...), legt das Logo
# darueber - es blendet nach einer Sekunde ein, steht und blendet wieder aus -
# und schreibt ein MP4 (H.264, 30 Bilder je Sekunde). Am Anfang und Ende
# blendet das Bild kurz aus Schwarz auf bzw. nach Schwarz ab.
#
#   blender -b --factory-startup --python video_schnitt.py -- BILDER LOGO ZIEL
#     BILDER  Ordner mit f0000.bmp, f0001.bmp, ...
#     LOGO    PNG mit Alphakanal
#     ZIEL    MP4-Datei
#
# Geschrieben fuer Blender 5.1 (Sequencer-API "strips"); aeltere Fassungen mit
# "sequences" gehen auch.

import bpy, os, sys

args = sys.argv[sys.argv.index("--") + 1:]
bilder_ordner, logo, ziel = args[0], args[1], args[2]

BPS = 30
LOGO_EIN = (1.0, 2.5)       # Sekunden: von - bis voll sichtbar
LOGO_AUS = (6.0, 7.5)       # Sekunden: ab - bis weg
LOGO_BREITE = 0.62          # Anteil der Bildbreite
SCHWARZ_EIN = 0.4           # Sekunden Aufblende am Anfang
SCHWARZ_AUS = 0.7           # Sekunden Abblende am Ende

bilder = sorted(f for f in os.listdir(bilder_ordner) if f.startswith("f") and f.endswith(".bmp"))
if not bilder:
    sys.exit("keine Bilder in " + bilder_ordner)
n = len(bilder)

szene = bpy.context.scene
r = szene.render
r.resolution_x, r.resolution_y, r.resolution_percentage = 1920, 1080, 100
r.fps = BPS
szene.frame_start, szene.frame_end = 1, n
# Farben 1:1 durchreichen - die Vorgabe (AgX) legt eine Filmkurve darueber,
# der Himmel wird dann grau.
szene.view_settings.view_transform = "Standard"
szene.view_settings.look = "None"

se = szene.sequence_editor_create()
strips = se.strips if hasattr(se, "strips") else se.sequences

def sek(s):
    return 1 + round(s * BPS)

# Flug
flug = strips.new_image("Flug", os.path.join(bilder_ordner, bilder[0]), channel=1, frame_start=1)
for f in bilder[1:]:
    flug.elements.append(f)
flug.blend_alpha = 0.0
flug.keyframe_insert("blend_alpha", frame=1)
flug.blend_alpha = 1.0
flug.keyframe_insert("blend_alpha", frame=sek(SCHWARZ_EIN))
flug.keyframe_insert("blend_alpha", frame=n - round(SCHWARZ_AUS * BPS))
flug.blend_alpha = 0.0
flug.keyframe_insert("blend_alpha", frame=n)

# Logo
lg = strips.new_image("Logo", logo, channel=2, frame_start=sek(LOGO_EIN[0]))
lg.frame_final_duration = sek(LOGO_AUS[1]) - sek(LOGO_EIN[0]) + 1
lg.blend_type = "ALPHA_OVER"
breite = bpy.data.images.load(logo).size[0]
s = 1920 * LOGO_BREITE / breite
try:
    lg.fit_method = "ORIGINAL"
except Exception:
    pass
lg.transform.scale_x = lg.transform.scale_y = s
for frame, a in ((sek(LOGO_EIN[0]), 0.0), (sek(LOGO_EIN[1]), 1.0),
                 (sek(LOGO_AUS[0]), 1.0), (sek(LOGO_AUS[1]), 0.0)):
    lg.blend_alpha = a
    lg.keyframe_insert("blend_alpha", frame=frame)

# Ausgabe: H.264 im MP4
ims = r.image_settings
if hasattr(ims, "media_type"):
    ims.media_type = "VIDEO"
ims.file_format = "FFMPEG"
r.ffmpeg.format = "MPEG4"
r.ffmpeg.codec = "H264"
r.ffmpeg.constant_rate_factor = "HIGH"
r.ffmpeg.ffmpeg_preset = "GOOD"
r.ffmpeg.audio_codec = "NONE"
r.filepath = ziel
r.use_file_extension = False

bpy.ops.render.render(animation=True)
print("FERTIG", ziel, n, "Bilder")

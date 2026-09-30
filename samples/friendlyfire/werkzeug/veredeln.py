"""Blender-Teil: Modelle des Friendly-Fire-Spiels veredeln.

Laeuft in Blender ohne Oberflaeche:

    blender -b --python veredeln.py -- modus eingabe ausgabe [schluessel=wert ...]

Der Renderer kennt nur eine Farbtextur (keine Normalen, kein Metall, keine
Leuchtflaechen). Was Tiefe macht, muss deshalb in der Farbe stehen. Hier wird
es hineingebacken:

  - Ambient Occlusion: Fugen und Spalten dunkler
  - Kantenlicht: ein Bevel-Shader, als Kantenmaske gebacken, hellt die Kanten
    in der Farbe des Materials auf
  - prozedurale Lagen im Objektraum (keine UV-Naehte): Facetten, Risse,
    Flecken, Koernung - je nach Material: Stein, blauer Kristall, Rost, Holz
  - Kontrast und Saettigung

Das Material eines Texels folgt aus seiner Farbe (Farbton, Saettigung,
Helligkeit): blau (Kristall), orange (Rost), braun (Holz), sonstige bunte
Farben (Lack) und Unbunt als Stein.

Modi:

  astra    Das Modell hat schon gute UVs und eine Farbtextur (basis=pfad.png).
           Ausgabe ist die neue Farbtextur als PNG; das Netz bleibt unberuehrt.
  neu      Das Modell (.glb mit Animationen, Knoten, Muendung) wird neu
           abgewickelt, alle Teile in einer gemeinsamen Textur. Die Farben
           der alten Textur (Kenneys Farbtafel) werden zuerst aufs neue
           Layout gebacken. Ausgabe ist eine .glb mit Textur als JPEG.

Schluessel: groesse (2048), samples (48), ao (0.7), ao_weite (0.3, relativ zur Groesse), kante (0.8),
radius (0.012, relativ zur Groesse des Modells), kontrast, saett, facette_b/_s, riss_b/_s, rost, sand,
facetten, risse, fleck (Zellen je Modellgroesse), basis.
"""

import os
import sys

import bpy
import numpy as np

args = sys.argv[sys.argv.index("--") + 1:]
modus, eingabe, ausgabe = args[0], os.path.abspath(args[1]), os.path.abspath(args[2])
ST = dict(a.split("=", 1) for a in args[3:])
f = lambda k, d: float(ST.get(k, d))
N = int(f("groesse", 2048))

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.fps = 60                # Bilder = Sekunden * 60, wie der Lader sie zaehlt
bpy.ops.import_scene.gltf(filepath=eingabe)
meshes = [o for o in sc.objects if o.type == "MESH"]
if modus == "astra":
    meshes.sort(key=lambda o: len(o.data.vertices), reverse=True)
    for o in meshes[1:]:
        bpy.data.objects.remove(o, do_unlink=True)
    meshes = meshes[:1]

sc.render.engine = "CYCLES"
sc.cycles.device = "CPU"
sc.cycles.samples = int(f("samples", 48))
sc.cycles.use_denoising = False


# Masse relativ zur Groesse des Modells (laengste Kante seiner Huelle): ein Bevel von
# 1,2 % und Zellen, die das Modell ein paarmal durchqueren, gleich ob Waffe oder Kiste
ecken = [o.matrix_world @ __import__("mathutils").Vector(c) for o in meshes for c in o.bound_box]
GROESSE = max(max(e[i] for e in ecken) - min(e[i] for e in ecken) for i in range(3))


def waehle(objs):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]


def bild_lesen(im):
    a = np.empty(im.size[0] * im.size[1] * 4, dtype=np.float32)
    im.pixels.foreach_get(a)
    return a.reshape(im.size[1], im.size[0], 4)


# --- 1. Neu abwickeln und die Farben der alten Textur aufs neue Layout backen ----
albedo = None
if modus == "neu":
    alt_uv = meshes[0].data.uv_layers[0].name
    for o in meshes:
        o.data.uv_layers.new(name="atlas")
        o.data.uv_layers.active = o.data.uv_layers["atlas"]
    waehle(meshes)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=np.radians(f("winkel", 50)), island_margin=f("luecke", 0.004))
    bpy.ops.object.mode_set(mode="OBJECT")

    ziel_bild = bpy.data.images.new("albedo", N, N, alpha=False)
    ziel_bild.colorspace_settings.name = "sRGB"
    for mat in {m for o in meshes for m in o.data.materials if m}:
        nt = mat.node_tree
        for n in nt.nodes:
            if n.type == "TEX_IMAGE":
                uvn = nt.nodes.new("ShaderNodeUVMap")
                uvn.uv_map = alt_uv
                nt.links.new(uvn.outputs["UV"], n.inputs["Vector"])
                n.interpolation = "Closest"         # Farbtafel: Felder, nicht weichzeichnen
        ziel = nt.nodes.new("ShaderNodeTexImage")
        ziel.image = ziel_bild
        nt.nodes.active = ziel
    waehle(meshes)
    bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR"}, margin=8, margin_type="EXTEND", use_clear=True)
    albedo = np.clip(bild_lesen(ziel_bild)[:, :, :3], 0, 1).copy()    # Byte-Bild: die Werte sind schon sRGB
    # das alte Material hat ausgedient
    for o in meshes:
        o.data.materials.clear()
        o.data.uv_layers.remove(o.data.uv_layers[alt_uv])

# --- 2. Backmaterial: Ziel-Bild + Emission, die das Backen ausliest --------------
mat = bpy.data.materials.new("bake")
mat.use_nodes = True
nt = mat.node_tree
nt.nodes.clear()
ausg = nt.nodes.new("ShaderNodeOutputMaterial")
emi = nt.nodes.new("ShaderNodeEmission")
nt.links.new(emi.outputs[0], ausg.inputs[0])
ziel = nt.nodes.new("ShaderNodeTexImage")
for o in meshes:
    o.data.materials.clear()
    o.data.materials.append(mat)


def backe(name, typ):
    im = bpy.data.images.new(name, N, N, alpha=False)
    im.colorspace_settings.name = "Non-Color"
    ziel.image = im
    nt.nodes.active = ziel
    waehle(meshes)
    bpy.ops.object.bake(type=typ, margin=8, margin_type="EXTEND", use_clear=True)
    return bild_lesen(im)[:, :, 0]


def lage(name, sock):
    nt.links.new(sock, emi.inputs["Color"])
    return backe(name, "EMIT")


welt = bpy.data.worlds.new("welt")
sc.world = welt
welt.light_settings.distance = f("ao_weite", 0.3) * GROESSE      # nur die Nachbarschaft verdunkelt
ao = backe("ao", "AO")

geo = nt.nodes.new("ShaderNodeNewGeometry")
bev = nt.nodes.new("ShaderNodeBevel")
bev.inputs["Radius"].default_value = f("radius", 0.012) * GROESSE
bev.samples = 8
pk = nt.nodes.new("ShaderNodeVectorMath")
pk.operation = "DOT_PRODUCT"
nt.links.new(bev.outputs["Normal"], pk.inputs[0])
nt.links.new(geo.outputs["Normal"], pk.inputs[1])
inv = nt.nodes.new("ShaderNodeMath")
inv.operation = "SUBTRACT"
inv.inputs[0].default_value = 1.0
nt.links.new(pk.outputs["Value"], inv.inputs[1])
kante = lage("kante", inv.outputs[0])

tc = nt.nodes.new("ShaderNodeTexCoord")


def prozedural(typ, skala, eigenschaft=None, **kw):
    n = nt.nodes.new(typ)
    if typ == "ShaderNodeTexVoronoi":
        n.voronoi_dimensions = "3D"
        if eigenschaft:
            n.feature = eigenschaft
    n.inputs["Scale"].default_value = skala
    for k, v in kw.items():
        n.inputs[k].default_value = v
    nt.links.new(tc.outputs["Object"], n.inputs["Vector"])
    return n


zelle = lage("zelle", prozedural("ShaderNodeTexVoronoi", f("facetten", 26.0) / GROESSE).outputs["Color"])
riss = lage("riss", prozedural("ShaderNodeTexVoronoi", f("risse", 34.0) / GROESSE, "DISTANCE_TO_EDGE").outputs["Distance"])
fleck = lage("fleck", prozedural("ShaderNodeTexNoise", f("fleck", 14.0) / GROESSE, Detail=8.0).outputs["Fac"])
sand = lage("sand", prozedural("ShaderNodeTexNoise", 120.0 / GROESSE, Detail=4.0).outputs["Fac"])

# --- 3. Farbe ---------------------------------------------------------------
if modus == "astra":
    im = bpy.data.images.load(os.path.abspath(ST["basis"]))
    im.colorspace_settings.name = "Non-Color"
    im.scale(N, N)
    albedo = bild_lesen(im)[:, :, :3].copy()
c = albedo


def hsv(rgb):
    mx, mn = rgb.max(-1), rgb.min(-1)
    d = mx - mn
    s = np.where(mx > 1e-5, d / np.maximum(mx, 1e-5), 0)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    dd = np.maximum(d, 1e-5)
    h = np.where(d < 1e-5, 0, np.where(mx == r, ((g - b) / dd) % 6,
                 np.where(mx == g, (b - r) / dd + 2, (r - g) / dd + 4))) / 6
    return h, s, mx


h, s, v = hsv(c)
blau = ((h > 0.5) & (h < 0.68) & (s > 0.35)).astype(np.float32)
orange = (((h < 0.13) | (h > 0.97)) & (s > 0.5) & (v > 0.45)).astype(np.float32)
holz = (((h < 0.13) | (h > 0.97)) & (s > 0.3) & (v <= 0.45)).astype(np.float32)
farbig = ((s > 0.3) & (blau + orange + holz == 0)).astype(np.float32)        # Gruen, Lila, Gelb ... (Lack)
stein = np.clip(1 - blau - orange - holz - farbig, 0, 1)

ao = np.clip(ao, 0, 1)[..., None]
kante = np.clip(np.clip(kante, 0, 1) * f("kante_staerke", 6.0), 0, 1)[..., None]
out = c.copy()
out *= (1 - f("ao", 0.7)) + f("ao", 0.7) * ao                                   # Fugen
licht = (blau[..., None] * np.array([0.75, 0.92, 1.0]) + orange[..., None] * np.array([1.0, 0.85, 0.45])
         + holz[..., None] * np.array([0.85, 0.6, 0.35]) + stein[..., None] * np.array([0.6, 0.72, 0.85])
         + farbig[..., None] * (0.5 * c + 0.5))
out = out + kante * f("kante", 0.8) * (licht - out * 0.3)                        # Kantenlicht

zelle, riss, fleck, sand = zelle[..., None], riss[..., None], fleck[..., None], sand[..., None]
rl = np.clip(1 - riss / f("riss_breite", 0.045), 0, 1) ** 1.5
rb, ob_, hb, sb, fb = blau[..., None], orange[..., None], holz[..., None], stein[..., None], farbig[..., None]
dunkel = np.clip(1.25 - v[..., None], 0.25, 1.0)        # heller Stein und Metall reissen kaum
out = out * (1 + (zelle - 0.5) * (f("facette_b", 0.55) * rb + f("facette_s", 0.35) * dunkel * sb + 0.15 * fb))   # Facetten
out = out * (1 - rl * (f("riss_b", 0.50) * rb + f("riss_s", 0.35) * dunkel * sb + 0.15 * ob_ + 0.12 * fb))         # Risse
rost = np.clip((fleck - 0.45) * 3.0, 0, 1)
out = out * (1 - rost * f("rost", 0.35) * ob_) + rost * f("rost", 0.35) * ob_ * np.array([0.55, 0.25, 0.08]) * 0.6
out = out * (1 + (sand - 0.5) * f("sand", 0.35) * (ob_ + sb + rb + fb))                         # Koernung
out = np.clip((out - 0.5) * f("kontrast", 1.12) + 0.5, 0, 1)
g = out.mean(-1, keepdims=True)
out = np.clip(g + (out - g) * f("saett", 1.15), 0, 1)

res = np.ones((N, N, 4), dtype=np.float32)
res[..., :3] = out
o = bpy.data.images.new("ergebnis", N, N)
o.colorspace_settings.name = "Non-Color"
o.pixels.foreach_set(res.ravel())

if modus == "astra":
    o.filepath_raw = ausgabe
    o.file_format = "PNG"
    o.save()
    print("FERTIG", ausgabe)
    sys.exit(0)

# --- 4. Modus neu: Textur ins Material, als JPEG exportieren -------------------
tmp = ausgabe + ".tmp.png"                  # ueber eine Datei: ein neu erzeugtes Bild gibt der Export schwarz aus
o.filepath_raw = tmp
o.file_format = "PNG"
o.save()
o = bpy.data.images.load(tmp)
o.colorspace_settings.name = "sRGB"         # die Werte sind schon sRGB; nur das Etikett aendert sich
o.pack()
neu = bpy.data.materials.new("veredelt")
neu.use_nodes = True
pb = neu.node_tree.nodes["Principled BSDF"]
pb.inputs["Roughness"].default_value = 1.0
pb.inputs["Metallic"].default_value = 0.0
tx = neu.node_tree.nodes.new("ShaderNodeTexImage")
tx.image = o
neu.node_tree.links.new(tx.outputs["Color"], pb.inputs["Base Color"])
for ob in meshes:
    ob.data.materials.clear()
    ob.data.materials.append(neu)
bpy.ops.export_scene.gltf(filepath=ausgabe, export_format="GLB", export_animations=True,
                          export_yup=True, export_image_format="JPEG", export_jpeg_quality=int(f("jpeg", 88)),
                          export_animation_mode="ACTIONS")
print("FERTIG", ausgabe)
os.remove(tmp)

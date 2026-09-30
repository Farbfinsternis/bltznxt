"""Blender-Teil von astra.py: das Netz der Astra-Waffe vereinfachen.

Laeuft in Blender ohne Oberflaeche, auf einer Kopie der Datei:

    blender -b --python astra_blender.py -- weapon.glb weapon_opt.glb [ziel-dreiecke]

Der Standardwuerfel faellt weg, dann werden erst ebene Flaechen zusammengefasst
(Planar Dissolve, an UV-Nahtstellen und Materialgrenzen geschnitten, damit die
Textur nicht verrutscht), dann kollabiert der Decimate-Modifikator auf die
Zieldreiecke. Texturen gehen unberuehrt durch.
"""

import sys

import bpy

args = sys.argv[sys.argv.index("--") + 1:]
quelle, ziel = args[0], args[1]
dreiecke = int(args[2]) if len(args) > 2 else 6000
winkel = float(args[3]) if len(args) > 3 else 2.0

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=quelle)

netze = [o for o in bpy.context.scene.objects if o.type == "MESH"]
netze.sort(key=lambda o: len(o.data.vertices), reverse=True)
waffe = netze[0]
for o in netze[1:]:
    bpy.data.objects.remove(o, do_unlink=True)
vorher = len(waffe.data.polygons)

bpy.context.view_layer.objects.active = waffe
waffe.select_set(True)
m = waffe.modifiers.new("eben", "DECIMATE")
m.decimate_type = "DISSOLVE"
m.angle_limit = winkel * 3.14159265 / 180
m.delimit = {"UV", "MATERIAL", "SHARP"}
bpy.ops.object.modifier_apply(modifier=m.name)
mitte = len(waffe.data.polygons)

# Dreiecke zaehlen, bevor der zweite Schritt rechnet
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.mesh.quads_convert_to_tris(quad_method="BEAUTY", ngon_method="BEAUTY")
bpy.ops.object.mode_set(mode="OBJECT")
tris = len(waffe.data.polygons)
if tris > dreiecke:
    m = waffe.modifiers.new("weniger", "DECIMATE")
    m.decimate_type = "COLLAPSE"
    m.ratio = dreiecke / tris
    m.use_collapse_triangulate = True
    bpy.ops.object.modifier_apply(modifier=m.name)
print("FLAECHEN", vorher, "->", mitte, "-> Dreiecke", tris, "->", len(waffe.data.polygons))

bpy.ops.export_scene.gltf(filepath=ziel, export_format="GLB", export_animations=False,
                          export_yup=True, export_image_format="AUTO")

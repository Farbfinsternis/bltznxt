; BUG-195 (Gruppe 3) - kleine Befehle: DirectInput, GraphicsLost,
; GraphicsBuffer, GfxDriverCaps3D, GfxMode3D*, ScanLine, VWait, BufferDirty,
; Stats3D (Eintrag 0 = gepruefte Kollisionsdreiecke) und MeshCullBox.
; Alle Werte am Original gemessen (2026-10-04). Joystick-Werte haengen vom
; angeschlossenen Geraet ab, ScanLine vom Monitor - daher nur grob geprueft.

Graphics3D 320,240,32,2
Print "DI " + DirectInputEnabled()
EnableDirectInput 0 : Print "DI0 " + DirectInputEnabled()
EnableDirectInput 1 : Print "DI1 " + DirectInputEnabled()
Print "lost " + GraphicsLost()
Print "gb " + (GraphicsBuffer() = BackBuffer())
SetBuffer FrontBuffer() : Print "gbf " + (GraphicsBuffer() = FrontBuffer()) : SetBuffer BackBuffer()
Print "caps " + GfxDriverCaps3D()
Print "m3d " + (CountGfxModes3D() > 0) + GfxMode3D(1) + GfxMode3DExists(640,480,32) + GfxMode3DExists(641,480,32) + GfxDriver3D(1)
s = ScanLine() : Print "scan " + (s >= 0)
VWait : BufferDirty BackBuffer() : Print "vwait ok"
; Stats3D
Print "st " + Stats3D(0) + " " + Stats3D(1) + " " + Stats3D(2)
cam = CreateCamera()
CameraClsColor cam,0,0,0
w = CreateCube() : PositionEntity w,0,0,5 : EntityType w,2 : EntityColor w,255,255,255 : EntityFX w,1
b = CreateSphere() : EntityType b,1 : EntityRadius b,0.5 : PositionEntity b,0,0,0
Collisions 1,2,2,2
UpdateWorld
Print "st0 " + Stats3D(0)
PositionEntity b,0,0,3.8
UpdateWorld
Print "st1 " + Stats3D(0) + " " + EntityZ(b)
UpdateWorld
Print "st2 " + Stats3D(0)
p = LinePick(0,0,-10,0,0,30)
Print "pick " + (p = w) + " " + Stats3D(0)
HideEntity b
; MeshCullBox
RenderWorld : Print "c0 " + Hex(ReadPixel(160,120))
MeshCullBox w,100,100,100,1,1,1
RenderWorld : Print "c1 " + Hex(ReadPixel(160,120))
MeshCullBox w,-1,-1,-1,2,2,2
RenderWorld : Print "c2 " + Hex(ReadPixel(160,120))
MeshCullBox w,0.5,0.5,0,0.2,0.2,0.2
RenderWorld : Print "c3 " + Hex(ReadPixel(160,120))
MeshCullBox w,0,0,0,-1,-1,-1
RenderWorld : Print "c4 " + Hex(ReadPixel(160,120))
Print "ok"

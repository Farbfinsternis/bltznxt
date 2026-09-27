xof 0302txt 0032

// 3D-19: zwei Frames mit Netz, Schluessel fuer Drehung, Lage und Skalierung.
// Handgeschrieben; siehe tests/test_3d19_animation_x.bb.

Frame Arm {
 FrameTransformMatrix {
  1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,2.0,0.0,1.0;;
 }
 Mesh {
  3;
  0.0;0.0;0.0;,
  1.0;0.0;0.0;,
  0.0;1.0;0.0;;
  1;
  3;0,1,2;;
 }
 Frame Hand {
  FrameTransformMatrix {
   0.0,0.0,-1.0,0.0, 0.0,1.0,0.0,0.0, 1.0,0.0,0.0,0.0, 3.0,0.0,0.0,1.0;;
  }
  Mesh {
   4;
   0.0;0.0;0.0;,
   1.0;0.0;0.0;,
   1.0;1.0;0.0;,
   0.0;1.0;0.0;;
   1;
   4;0,1,2,3;;
  }
  Frame Finger {
   FrameTransformMatrix {
    1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 1.0,0.0,0.0,1.0;;
   }
  }
 }
}

AnimationSet Winken {
 Animation {
  {Arm}
  AnimationKey {
   0;
   3;
   0;4;1.0,0.0,0.0,0.0;;,
   10;4;0.707107,0.0,0.707107,0.0;;,
   20;4;0.0,0.0,1.0,0.0;;;
  }
  AnimationKey {
   2;
   2;
   0;3;0.0,2.0,0.0;;,
   20;3;0.0,4.0,0.0;;;
  }
 }
 Animation {
  {Hand}
  AnimationKey {
   1;
   2;
   0;3;1.0,1.0,1.0;;,
   24;3;2.0,1.0,1.0;;;
  }
 }
}

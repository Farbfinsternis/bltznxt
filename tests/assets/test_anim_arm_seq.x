xof 0302txt 0032

// 3D-19: zweite Sequenz fuer test_anim_arm.x (LoadAnimSeq). Ohne Netze; der
// Frame Finger fehlt, dafuer gibt es einen Frame Extra, den das Modell nicht hat.

Frame Arm {
 FrameTransformMatrix {
  1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0;;
 }
 Frame Hand {
 }
}
Frame Extra {
}

AnimationSet Nicken {
 Animation {
  {Hand}
  AnimationKey {
   0;
   2;
   0;4;1.0,0.0,0.0,0.0;;,
   8;4;0.866025,0.5,0.0,0.0;;;
  }
  AnimationKey {
   2;
   2;
   0;3;3.0,0.0,0.0;;,
   8;3;3.0,1.0,0.0;;;
  }
 }
 Animation {
  {Extra}
  AnimationKey {
   2;
   1;
   5;3;9.0,9.0,9.0;;;
  }
 }
}

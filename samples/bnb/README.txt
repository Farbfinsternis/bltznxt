Blox'n'Balls
============

A 3D block breaker on a table: remaster of the Blitz3D game blox-n-balls
(2003), rewritten from scratch as a sample for BLTZNXT:
https://github.com/Farbfinsternis/bltznxt

Version 0.1: table, paddle, ball, blocks that take one to three hits,
bumpers and obstacles, a clock per level, three skill levels, high scores
and an autosave, the extras of the special blocks, an inventory, coins
and a shop between the levels. Ten levels: the five of the original and
five new ones. Paddle, ball and some models are still placeholders until
the new artwork arrives; a level editor follows.


Building and starting
---------------------

  blitzcc bloxnballs.bb -o bloxnballs
  bloxnballs.exe

Requirements: Windows 64 bit, a graphics card with OpenGL 3.3.


Controls
--------

  Mouse          move the paddle, left button launches the ball
  Space          nudge the table
  Tab            ball camera on / off
  Mouse wheel    select an item of the inventory (also Up / Down);
                 behind the last item there is an empty place
  Right button   use the item (also Ctrl)
  Esc            menu (the game waits, "Resume" continues)
  F12            screenshot
  Gamepad        stick moves, button 1 launches, button 4 nudges,
                 buttons 5 / 6 select an item, button 2 uses it


Command line
------------

  -debug         F1 clears the level, F2 adds a ball, frame time on screen
  -autoplay      starts a game at once and plays by itself
  -level <n>     first level for -autoplay
  -skill <n>     0 easy, 1 normal, 2 hard
  -quit <s>      quits after s seconds and saves test_quit.bmp
  -ballcam       starts with the ball camera on
  -shots <s>     saves test_001.bmp, test_002.bmp ... every s seconds
  -shop          starts in the shop with 20 coins, then plays by itself


Files
-----

  assets/levels  the levels as text, format in assets/levels/README.txt
  werkzeug/      convert_bbl converts the levels of the original game

Settings (bloxnballs.cfg), high scores (scores.dat) and the autosave
(save.dat) are written next to the executable.


Extras
------

  Multiball      3, 4 or 5 balls in all; the extra balls are green. While
                 green balls are on the table, losing the white ball
                 costs nothing, it comes back onto the paddle.
  Speed ball     a push to four times the speed that dies away in 4 s
  Small / wide   half / double paddle for 10 s
  Tiny / big     smaller ball / a big ball that breaks blocks at once, 10 s
  Slow motion    slower balls for 10 s
  Score x2       double points for 10 s
  Time +15 / -15, extra ball, new blocks, atomic ball (into the
  inventory: the next block it hits blows up everything around it)

The paddle bends the ball: moving it while the ball hits adds to the
angle the hit point gives.


Coins and shop
--------------

Broken blocks sometimes leave a coin where they stood; roll the ball over
it to pick it up. Coins left at the end of a level are lost. Coins do not
count for the score. After each level the shop sells items for the
inventory: extra ball, atomic ball, multiball, big ball, score x2, wide
paddle, slow motion and time +15. The autosave keeps coins and inventory.


Credits
-------

Game, code and graphics: Farbfinsternis
Music and sounds: the original soundtrack of blox-n-balls (2003)
Font: Road Rage, (c) 2010 The Road Rage Project Authors, SIL Open Font
License 1.1 (assets/fonts/OFL.txt)

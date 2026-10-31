Blox'n'Balls
============

A 3D block breaker on a table: remaster of the Blitz3D game blox-n-balls
(2003), rewritten from scratch as a sample for BLTZNXT:
https://github.com/Farbfinsternis/bltznxt

Version 0.1 (in progress): table, paddle, ball, blocks that take one to
three hits, bumpers and obstacles, a clock per level, three skill levels,
high scores and an autosave. The five levels of the original are included.
Graphics are placeholders until the new artwork arrives; the shop, the
extras and a level editor follow.


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
  Esc            menu (the game waits, "Resume" continues)
  F12            screenshot
  Gamepad        stick moves, button 1 launches, button 4 nudges


Command line
------------

  -debug         F1 clears the level, F2 adds a ball, frame time on screen
  -autoplay      starts a game at once and plays by itself
  -level <n>     first level for -autoplay
  -skill <n>     0 easy, 1 normal, 2 hard
  -quit <s>      quits after s seconds and saves test_quit.bmp


Files
-----

  assets/levels  the levels as text, format in assets/levels/README.txt
  werkzeug/      convert_bbl converts the levels of the original game

Settings (bloxnballs.cfg), high scores (scores.dat) and the autosave
(save.dat) are written next to the executable.

Blox'n'Balls - level format
===========================

One text file per level: levelNN.txt, played in order of NN.
Lines starting with ";" are comments. The header holds "key: value" lines,
the grid follows the line "grid:".

  name:  shown when the level starts
  time:  seconds on the clock (scaled by the skill level)
  speed: base speed of the ball in table units per second (8 - 16 is sane)
  music: file in assets/music (optional)
  grid:  one line per row, one character per cell; the first line is the row
         at the back wall. All lines should have the same length; the table
         is always the same size, cells shrink to fit wider or deeper grids.

Cell codes
----------

  .        empty
  1 2 3    block that takes 1, 2 or 3 hits

  Special blocks (break with one hit, then do their extra; timed extras
  last 10 s, score x2 lasts 20 s):

  P Q R    points 50 / 75 / 100
  M N O    multiball +1 / +2 / +3 (new balls fly off the block)
  E        extra ball
  A        atomic ball - goes into the inventory; used, the next block
           the ball hits blows up with everything around it
  S        speed ball (x1.5)
  B        new blocks - fills the empty cells in the 5 x 5 cells around
  s        small paddle
  t        tiny ball
  W        wide paddle (twice for an even wider one)
  L        slow motion (x0.6)
  G        big ball - breaks every block with one hit
  X        score x2
  T        time +15 s
  U        time -15 s

  Indestructible (do not count for clearing the level):

  o        bumper - kicks the ball away, 1 point
  |        obstacle, cylinder
  #        obstacle, block
  /        obstacle, 45 degrees

The five original levels were converted with werkzeug/convert_bbl.

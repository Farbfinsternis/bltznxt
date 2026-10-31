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

  Special blocks (break with one hit; their extras arrive after 0.1,
  until then they only give points):

  P Q R    points 50 / 75 / 100
  M N O    multiball +1 / +2 / +3
  E        extra ball
  A        atomic ball
  S        speed ball
  B        new blocks
  s        small paddle
  t        tiny ball
  W        wide paddle
  L        slow motion
  G        big ball
  X        score x2
  T        time +15 s
  U        time -15 s

  Indestructible (do not count for clearing the level):

  o        bumper - kicks the ball away, 1 point
  |        obstacle, cylinder
  #        obstacle, block
  /        obstacle, 45 degrees

The five original levels were converted with werkzeug/convert_bbl.

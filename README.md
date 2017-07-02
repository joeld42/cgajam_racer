================================================
  
   ____   __    ___  ____  ____   ___   ___  ___ 
  (  _ \ / _\  / __)(  __)(  _ \ / _ \ / __)/ __)
   )   //    \( (__  ) _)  )   /(__  (( (__( (__ 
  (__\_)\_/\_/ \___)(____)(__\_)  (__/ \___)\___)

                --=== Racer4cc ===--

================================================
by Joel Davis 
Twitter: @joeld42

Soundtrack by Scott DeVaney

===[ DESCRIPTION ]===============================

Imagine modern graphics cards could only support 
the origional four color IBM-PC CGA palette. This
game is an experiment to see what that would be 
like. It was created as part of #CGAJAM, although
I did not finish in time for the CGAJAM deadline.

===[ LINKS ]=====================================

CGAJAM:
https://itch.io/jam/cga-jam

Racer4cc source on github
https://github.com/joeld42/cgajam_racer

Built using the Raylib videogames library
http://www.raylib.com/
Raylib is a great library to learn game programming, or
for prototyping or game jams.

Uses the SoLoud library for audio:
http://sol.gfxile.net/soloud/index.html

Uses Rocket sync and emoon's GL sync tracker for the 
title screen/attract mode sequencing
https://github.com/emoon/rocket

===[ CONTROLS ]=====================================

Game Controls:
- Left/Right to steer
- Up to accellerate, down to brake

Complete three laps to finish the race. Your best
time, and best lap time, are shown on the title screen.

===[ DEBUG MODE ]===================================

Additionally there are some "Debug Mode" keys added
during development. I left them in because some of
them are interesting or useful. 

Press '1' to enable Debug Keys (and show FPS counter).

Debug Keys during game:
- 'Z' reset racer to starting line
- 'X' clear out racer velocity
- 'L' increment lap count

Other Debug Keys:
- 'D' take screenshot (will save cgaracer000N.png in current dir)
- 'F' Generate random Sound effect and print its seed (this was my sound design pipeline)

- '0' Show framebuffer before CGA pixelation effect
- '9' Toggle low-res buffer and CGA Pixalation effects
- '8' Toggle Framebuffer views. This will show the unpixelated render, the "material" render,
      the final combined pixelated view, and some debug graphs of speed etc,
- '7' Show gradient debug card
- '5' Generate a random track, print seed

- 'E' Toggle track editing mode. Move track spline control points with mouse
- 'B' Rebuild track mesh from spline
- 'P' Print track control points (to cut/paste into track.cpp)



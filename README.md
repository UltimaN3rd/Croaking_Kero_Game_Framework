# Kero Game Framework

This is Nick Walton's game backend framework. I've created and am using this to program small games from scratch for Windows, Linux and MacOS (aspiring to support console platforms in future).

## Warning

**This is not a set-and-forget, production-ready framework like SDL and Raylib. Do not just download this and expect to start making games.
I have released this into the public domain for educational purposes and the public good. I hope this will be useful to you as a learning resource and starting point for doing your own from-scratch programming. This is not intended to be a widely used programming library. While I appreciate bug reports and suggestions for improvements, I am not intending to coach people through using this code.

## Why I made this framework

When programming games in the past, I've listened to the advice, "Don't reinvent the wheel," and, "You probably can't do it better than X, so just use X." This advice is intended to save people time and get straight to productive game programming, but I found it difficult to become a good enough programmer while skipping so many foundational parts of the code. I also felt demotivated working on games that I ultimately may be unable to maintain - like the multitude of fantastic Flash-based games from back in the day. I finally decided to build my games from the ground up and created this framework. Now I've completed my first game (launching on Steam soon), and have become a much better and faster programmer. It was not easy to get this far, and I've encountered a significant lack of resources on how to write this kind of code, so I'm putting this in the public domain as an example for others to get over some hurdles.

## Work in progress

This code is not finished or final. I have completed a game with it, so it's functional, but it is not significantly field-tested. The API is not locked; frankly there are no guarantees that anything about this code will be the same tomorrow. I intend to fix any bugs that people encounter when running my games built with this framework, and add features to it such as key rebinding, controller support and anything else my future games require.

## Architecture

This framework is multi-threaded, with separate threads for operating system events, sound, rendering and logic. The main thread initializes the window, logging, etc., starts the sound, rendering and logic threads, then enters an event loop which it will continue until the program shuts down. The other threads have their own initilizations, then monitor a "quit" variable controlled by the main thread while they perform their functions. The sound and rendering threads use command buffer architectures - the game code calls functions like SoundFXPlay() and Render_Sprite() to add commands to their buffers, which they will carry out in their next loop iteration.
The rendering thread determines the monitor refresh rate, then locks to that rate. Every loop, it selects the latest complete "render state" buffer which contains the latest complete game frame's rendering commands and executes all the commands to produce a frame.
The sound thread is more integrated with the OS, but overall it:
-Waits for a callback from the native OS sound library.
-Gives the OS 5ms of prepared, compressed sound samples.
-Executes any available commands.
-Refills the previously used sample buffer with 5ms of uncompressed samples.
-Compresses the other 5ms sample buffer.
-Repeat.
Compression is done with a 5ms lookahead compression algorithm, using the greatest sample magnitude of the current and next buffer. The greater value is lerped to over a 5ms period, and always reached before compression of the buffer containing the peak value begins compression.
The logic thread is where the game code is all run. It runs at a fixed 120Hz. It takes input events from a ring buffer filled by the main thread and maintains its own arrays for frame-by-frame inputs for the game logic. Whenever the "state" is changed, it runs that state's Init() function, then runs the state's Update() function every frame. On startup the 0th state is automatically started, and from there it's up to the game logic within that state's Update() or Init() function to change the state when appropriate.
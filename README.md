  
  
## build dependencies  

- recursive submodule clone to get sokol and sokol-tools-bin  
- clang  
- [tup](https://gittup.org/tup/win32/tup-latest.zip)  (and ideally have it in your PATH)
- emsdk (only for web building) - https://emscripten.org/docs/getting_started/downloads.html (you'll also need the emsdk in your PATH environment variable so activate_dev.bat can pick up the emscripten activation script. Or use emsdk.bat activate --permanent)  

once dependencies are installed, run `activate_dev.bat` and then `firsttime_init.bat`
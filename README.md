# ideas
- topiary business
    - take care of - i.e. trim & water hedges, shape them to what the client wants
    - must plant and sell hedges, can upgrade trimmers, fertilizer, space to grow, etc


  
## build dependencies  

- recursive submodule clone to get sokol and sokol-tools-bin  
- clang  
- [tup](https://gittup.org/tup/win32/tup-latest.zip)  (and ideally have it in your PATH)
- emsdk (only for web building) - https://emscripten.org/docs/getting_started/downloads.html (you'll also need the emsdk in your PATH environment variable so activate_dev.bat can pick up the emscripten activation script. Or use emsdk.bat activate --permanent)  

once dependencies are installed, run `activate_dev.bat` and then `firsttime_init.bat`



# TODO:
- ~~transfer web from webgl/opengl-es to webgpu (wgpu supports storage buffers and ogles doesn't)~~
- ~~try vertex pulling again~~
- ~~move meshlet data to gpu buffers~~
- ~~render meshlet assignments~~
- ~~extract camera frustum planes~~
- ~~extract meshlet bounding spheres~~
- ~~interleave vertex data~~
- render meshlet bounding spheres
- render arbitrary frustum
- run frustum cull on all meshlet spheres each frame
- display culling
    - actually not render culled meshlets
    - just color those meshlets differently based on cull
- occlusion culling? 
- manual web data packer (file_packer from emscripten) and customize js loading code
    - also add loading screen to desktop build. so web and desktop build stream in asset on boot and load it when rdy
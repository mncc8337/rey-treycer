# Rey Treycer
a simple header only ray tracer library running on CPU i made when bored
> [!NOTE]  
> the library can only render the image and store it on memory. to save it as file you need to use external library. see `examples/image.h` for how it is done using `stb`.
## examples
to build the examples on `./examples` you will need  
- [SDL2](https://www.libsdl.org/)
- [imgui](https://github.com/ocornut/imgui) and [stb](https://github.com/nothings/stb), which are availabled as submodules
after installed all dependencies just cd into `./examples` then run `make all` to build  
you can run the binary (`gui` and `no-gui`) with `cornell`, `textures` or `all` as argument to switch the scene. for example `gui textures`  
all generated images are on `./examples/imgs`  
> [!WARNING]  
> `gui.h` is very messy and has many bad practises, you should not follow what are written here!
## usage
i will add this tomorrow i swear
## known bugs
havent found any, yet.
## TODO
- [ ] remake smoke
- [ ] add real matrix maths
- [ ] add BVH
- [ ] add more material options: normals/roughness based on image
- [ ] remove trash codes

## gallery
<p>
    <img src="imgs/scene-8.bmp" width=47%>
    <img src="imgs/defocus-effect-2.bmp" width=47%>
    <img src="imgs/defocus-effect-1.bmp" width=47%>
    <img src="imgs/refraction-2.bmp" width=47%>
    <img src="imgs/scene-6.bmp" width=47%>
    <img src="imgs/scene-0.bmp" width=47%>

## learning resources
- https://raytracing.github.io

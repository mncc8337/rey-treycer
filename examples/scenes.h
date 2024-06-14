#ifndef SCENES_H
#define SCENES_H

#include "rey-treycer.h"
#include "image.h"

// object that need to be access as pointer like Object, Texture (later Material)
// needed to be created with `new` keyword if defined in local scope to keep it exist
// if created in main function you dont need the new keyword because it will remain
// until the end of the program

inline void cornell_box(ReyTreycer& rt) {
    Mesh plane = load_mesh_from("default_model/plane.obj");
    plane.set_scale({5, 5, 5});

    ColorTexture* tex_red = new ColorTexture;
    tex_red->color = RED;
    ColorTexture* tex_green = new ColorTexture;
    tex_green->color = GREEN;
    ColorTexture* tex_white = new ColorTexture;
    tex_white->color = WHITE;

    Material mat_red;
    mat_red.texture = tex_red;

    Material mat_green;
    mat_green.texture = tex_green;

    Material mat_white;
    mat_white.texture = tex_white;

    Material mat_light;
    mat_light.emit_light = true;
    mat_light.emission_strength = 5.0f;
    mat_light.texture = tex_white;

    Mesh* floor = new Mesh; *floor = plane;
    floor->set_position({0, -5, 0});
    floor->set_material(mat_white);
    floor->update_material();
    floor->calculate_AABB();
    rt.add_object(floor);

    Mesh* ceil = new Mesh; *ceil = plane;
    ceil->set_rotation({M_PI, 0, 0});
    ceil->set_position({0, 5, 0});
    ceil->set_material(mat_white);
    ceil->update_material();
    ceil->calculate_AABB();
    rt.add_object(ceil);

    Mesh* wall_back = new Mesh; *wall_back = plane;
    wall_back->set_rotation({M_PI/2, 0, 0});
    wall_back->set_position({0, 0, -5});
    wall_back->set_material(mat_white);
    wall_back->update_material();
    wall_back->calculate_AABB();
    rt.add_object(wall_back);

    Mesh* wall_front = new Mesh; *wall_front = plane;
    wall_front->set_rotation({M_PI/2, M_PI, 0});
    wall_front->set_position({0, 0, 5});
    wall_front->set_material(mat_white);
    wall_front->update_material();
    wall_front->calculate_AABB();
    rt.add_object(wall_front);

    Mesh* wall_red = new Mesh; *wall_red = plane;
    wall_red->set_rotation({0, 0, -M_PI/2});
    wall_red->set_position({-5, 0, 0});
    wall_red->set_material(mat_red);
    wall_red->update_material();
    wall_red->calculate_AABB();
    rt.add_object(wall_red);

    Mesh* wall_green = new Mesh; *wall_green = plane;
    wall_green->set_rotation({0, 0, M_PI/2});
    wall_green->set_position({5, 0, 0});
    wall_green->set_material(mat_green);
    wall_green->update_material();
    wall_green->calculate_AABB();
    rt.add_object(wall_green);

    Mesh* light = new Mesh;
    *light = load_mesh_from("default_model/cube.obj");
    light->set_scale({2.5f, 0.1f, 2.5f});
    light->set_position({0, 5, 0});
    light->set_material(mat_light);
    light->update_material();
    light->calculate_AABB();
    rt.add_object(light);
}

inline void all_textures(ReyTreycer& rt) {
    // dice
    ImageTexture* dice_tex = new ImageTexture;
    dice_tex->pixel_data = load_image("texture/dice.png",
                                      &(dice_tex->image_width), &(dice_tex->image_height),
                                      &(dice_tex->channels));
    Material cube_mat;
    cube_mat.texture = dice_tex;

    Mesh* cube = new Mesh;
    *cube = load_mesh_from("default_model/cube-uv.obj");
    cube->set_material(cube_mat);
    cube->update_material();
    cube->set_position({1.4, 0, 0});
    cube->set_rotation({0.5, -2.33, 0});
    // after setting mesh transformation you need to run calculate_AABB()
    cube->calculate_AABB();
    rt.add_object(cube);

    ProceduralTexture* checker_tex = new ProceduralTexture;
    // see all pre-defined function in rey-treycer.h tail
    checker_tex->set_function(&checker); 
    Material sphere_mat;
    sphere_mat.texture = checker_tex;

    Sphere* sphere = new Sphere;
    sphere->set_material(sphere_mat);
    sphere->set_position({0, 3, 0});
    rt.add_object(sphere);

    ColorTexture* color_tex = new ColorTexture;
    color_tex->color = WHITE;
    Material dodeca_mat;
    dodeca_mat.texture = color_tex;

    Mesh* dodecah = new Mesh;
    *dodecah = load_mesh_from("default_model/dodecahedron.obj");
    dodecah->set_material(dodeca_mat);
    dodecah->update_material();
    dodecah->set_position({-2, 0, 0});
    dodecah->set_rotation({0.5, -1.33, 0});
    dodecah->calculate_AABB();
    rt.add_object(dodecah);
}

#endif

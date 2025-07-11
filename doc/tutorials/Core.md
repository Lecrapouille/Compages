# OpenGLCppWrapper Tutorial

## How to use this API? An explained 'Hello-world' example

This section will treat on how to use this API. This is a small tutorial explaining the template for your projects. Indeed, the pseudo-C++ code shown in this tutorial will be the same for many of your 3D applications that you will make. As proof, I made numerous [examples](../../examples/README.md) included with the code source of this library, which follow the pseudo-code of this tutorial. My examples are greatly inspired by several OpenGL tutorials found on the internet (see their README for credits). You can compare the length of the code.

In this document, I consider that the reader has basic knowledge of modern OpenGL (which is my case). There is nothing particularly difficult with this tutorial because everything is already largely explained in the first chapters of any OpenGL tutorials foundable on the internet [here](https://learnopengl.com/) or [here](http://www.opengl-tutorial.org/) or [youtube](https://www.youtube.com/playlist?list=PLRIWtICgwaX0u7Rf9zkZhLoLuZVfUksDP).

Before showing the C++ pseudo-code using OpenGLCppWrapper, we have, first, to write a basic fragment and vertex shader named `my_fragment_shader.glsl` and `my_vertex_shader.glsl`. The geometry shader is not used here but you can add one. OpenGLCppWrapper has been conceived to develop OpenGL applications by starting from shader code and finish by filling VBOs and setting path of desired textures. This is the opposite way of thinking compared to the way tutorials make you learn OpenGL (see the one I cited in the previous paragraph).

##### my_fragment_shader.glsl:

In this fragment shader, we simply pick the color of the texture (texel) and blend it with a uniform color. Uniforms are kind of global variables, while `in` and `out` are the inputs and outputs of the OpenGL pipeline connecting CPU, shaders and framebuffers. Point of view of the fragment shader `in` are outputs of the vertex shader and `out` is the final destination.

```
01: #version 330 core
02:
03: uniform sampler2D texID; // Texture sampler
04: in vec2 texUV;           // Texture coordinate
05: in vec4 texColor;        // Texture color
06: out vec4 fragColor;      // Final color
07:
08: void main()
09: {
10:     fragColor = texture(texID, texUV) * texColor;
11: }
```

##### my_vertex_shader.glsl:

In this vertex shader, we pass to the fragment shader the texel and the global color. We applied to vertices position a [Model-View-Projection matrix](http://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices) (change of basis). For the fun, we applied an additional scaling factor. Again, this kind of basic shaders is largely described on the internet. Point of view of vertex shader `in` are outputs of the CPU application and `out` are inputs of the fragment shader.

```
01: #version 330 core
02:
03: uniform float scale;     // Global scaling
04: uniform mat4 model;      // Model matrix
05: uniform mat4 view;       // View matrix
06: uniform mat4 projection; // Projection matrix
07: uniform vec4 color;      // Global color
08: in vec3 position;        // Vertex position
09: in vec2 UV;              // Vertex texture coordinates
10: out vec2 texUV;          // Interpolated fragment texture coordinates
11: out vec4 texColor;       // Interpolated fragment color
12:
13: void main()
14: {
15:     // Varying variables
16:     texUV = UV;
17:     texColor = color;
18:
19:     // Final position
20:     gl_Position = projection * view * model * vec4(scale * position, 1.0);
21: }
```

##### main.cpp:

Here the interesting part of this document using the OpenGLCppWrapper API. The code is written in pseudo-C++ code to be less boring to read. Explanations come after.

```
01: GLProgram prog;
02: GLVAO vao1; GLVAO32 vao2;
03:
04: // Init phase:
05: {
06:      // Load shaders code
07:      GLVertexShader vertex_shader; vertex_shader.read("my_vertex_shader.glsl");
08:      GLFragmentShader fragment_shader; fragment_shader.read("my_fragment_shader.glsl");
09:
10:      // Compile and link shaders into a GLProgram
11:      if (!prog.compile(vertex_shader, fragment_shader))
12:      {
13:            std::cerr << "Failed compiling OpenGL program. Reason was '" << prog.strerror() << "'" << std::endl;
14:            return false;
15:      }
16:
17:     // Init shader uniforms
18:     prog.scalarf("scale") = 1.0f;
19:     prog.vector4f("color") = Vector4f(0.8f, 0.2f, 0.8f, 0.8f);
20:
21:     // Populate VBOs and textures for the 1st VAO
22:     if (!prog.bind(vao1))
23:     {
24:            std::cerr << "Failed binding. Reason was '" << prog.strerror() << "'" << std::endl;
25:            return false;
26:     }
27:
28:     // Define a 3D model (for example a cube)
29:     vao1.vector3f("position") = { Vector3f(1.0f, 2.0f, 3.0f) ... Vector3f(...) };
30:     vao1.vector2f("UV") = { Vector2f(1.0f, 1.0f) ... Vector2f(...) };
31:
32:     // Apply a 2D texture to the 3D model
33:     if (!vao1.texture2D("texID").load("my_texture1.jpg"))
34:     {
35:          std::cerr << "Failed loading texture" << std::endl;
36:          return false;
37:     }
38:
39:     // Populate VBOs and textures for the 2nd VAO
40:     prog.bind(vao2);
41:
42:     // Define another 3D model (for example a plane) with different texture and using index
43:     vao2.vector3f("position") = { ... };
43:     vao2.vector2f("UV") = { ... };
44:     vao2.texture2D("texID").load("my_texture2.tga");
45:     vao2.index() = { ... };
46: }
47:
48: // Runtime phase:
49: {
50:     // Draw model 1 with implicit mode, first and count vertices
51:     prog.matrix44f("model") = Matrix44f(...);
52:     vao1.draw(Mode::TRIANGLES, 0, 36);
53:
54:     // Draw model 2 with implicit first and count vertices
55:     prog.matrix44f("model") = Identity44f;
57:     vao2.draw(Mode::TRIANGLES);
58:
59:     // Alternative ways:
60:     vao1.draw(Mode::TRIANGLES); vao1.draw();
61:     vao2.draw();
62: }
63:
64: // Camera management
65: {
66:     prog.matrix44f("projection") = Matrix44f(...);
67:     prog.matrix44f("view") = Matrix44f(...);
68: }
```

### Explanations

**Initialization phase:** This code is usually made once and in the `onSetup()` method (for example called by a GLWindow).

* Global overview:
GL*x*Shader, GLProgram, and GLVAO are some OpenGL objects that OpenGLCppWrapper is exposing to the developer (where *x* is referring either to Fragment or Vertex or Geometry). A GL*x*Shader holds the GLSL code source and compile it (in future it will may offer macro processing features). A GLProgram holds at least two GLxShaders (vertex and fragment) and links them. Once compiled, a GLProgram holds internally and in privacy locations of shader uniforms, shader attributes and texture samplers. A GLVAO can be considered as the OpenGL handler to your 3D model: it is an interface between CPU data and GPU data by holding VBOs and textures. You can modify data of textures and data of VBOs: the API will transfer modified values to your graphical card.

* Lines 07-08:
You have to load and compile your GLSL code given as vertex and fragment shader (and optionally geometry shader). You can either refer the file path with `read(std::string const&)` or directly pass the code as a string with operators `>>` and `<<` (not shown here).

* Lines 11-15, 51, 55:
You have to compile and link shaders into the class GLProgram. If shaders contained syntax errors the `compile()` will return `false` and errors can be shown with `strerror()`. If your OpenGL has been compiled with success, the GLProgram will create its own private dictionary of shader uniforms, attributes, and samplers from shaders variable names (`uniform`, `in` and `out`). In this example, they are:
  - uniforms: `scale, model, view, projection, color`.
  - attributes: `position, UV`.
  - samplers: `texID`.

* **Note:** You can repeat lines 07-15 for adding more GLPrograms and compile more shaders code.

* Lines 18-19:
Shader uniforms can be initialized directly after a GLProgram has been compiled with success. Indeed, they act like constant values inside shaders. Shader attributes and shader samplers need a bound VAO to be initialized (as explained after). Shader uniforms data are reachable through methods like `scalarf(const char*)` for float, `vector2f(const char*)`, `vector3f(const char*)`, `vector4f(const char*)` for vectors, and `matrix22f(const char*)`, `matrix33f(const char*)`, `matrix44f(const char*) ...` for matrices. For integers and double replace `f` by `i` and `g`. The name shall be the GLSL **EXACT** variable name (and respect the case), else an `GL::Exception` will be thrown.

* Line 22-26 and 33-37:
To initialize your 3D model, you, first, have to use a VAO and bind it to the desired GLProgram through the `GLProgram::bind(GLVAO&)` method. The GLProgram, wit this method, helped by its internal list of shader attributes and samplers, will populate VBOs and textures inside the bound VAO instance. Shader attributes and samplers give their name to VBOs and textures. Binding is mandatory and not calling it or trying to bind an already bound VAO to a different GLProgram will throw an `GL::Exception`. Note that the second time that `bind()` is called to the same GLProgram.

* Line 29-45:
In our example, `vao1` and `vao2` will have a VBO named `position` and `UV` and a texture named `texID`. VBOs and textures are like `std::vector`: a CPU-GPU buffer for storing data of your 3d models (vertex position, texture coordinates, normals ...). All VBOs shall have the same size. Automatic checks of their sizes are made. Their content can be modified by the CPU: dirty data will be automatically transfered to GPU. They are reachable through methods like `scalarf(const char*)`, `vector3f(const char*)`, `matrix44f(const char*)` or `texture1D(const char*)`, `texture2D(const char*)`, `texture3D(const char*)`, `textureCube(const char*)`. The name shall be the GLSL **EXACT** variable name (and respect the case), else an `GL::Exception` will be thrown.

* Line 45:
In our example, `vao2` contrary to `vao1` has an index to reduce the number of vertices to give and replaced by an simple index. You can use `GLVAO8` or `GLVAO16` instead of `GLVAO32` which changes the type used for the index (uint8, uint16 or uint32).

* Lines 33-37:
Textures shall be initialized. Textures are uniforms but they are filled like attributes (need a bind VAO). Textures dimension depends on the sampler used in your GLSL code: the can be 1D, 2D, 3D or Cube. You can load jpeg, BMP, png, TGA ... files and for 2D textures, you have to use `load(const char*)` or their equivalent. These methods also accept to change the texture loader, else by default the SOIL library will be used. You can derive and implement yours.

* line 40-45:
Like for GLPrograms,you can create as many VAOs as desired. The number of VAOs only depends on how many 3D models you desire to create. You have to redo the same steps for filling VBOs and textures. In this example, two VAOs are bound to the same shader program but you can add more shader programs, for example when using framebuffers: one shader program for rendering the 3d scene to a texture and the second shader program for doing some post-treatment filters.

**Run-time phase:** This code is usually called by the `onPaint()` method (for example called by a GLWindow) for rendering your 3D scene.

* line 52, 57, 60-61:
Only bound VAO with filled VBO can draw a 3D model.

* line 60, 61:
Show different ways to draw models (with/without the number of vertices indicated, with/without index buffer, with/without the VAO...). You can draw triangles, striped triangles, lines, points or different types of primitives offered by OpenGL.

* line 51, 55:
Depending on your GLSL code, if you use [Model View Projection matrices](http://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/) (See any OpenGL tutorials concerning how this works). Use the model matrix to place your models inside the scene. Because in Core OpenGL, routines like `glPushMatrix()` and `glPopMatrix()` no longer exist, this API proposes to use a scene graph for more complex scenes (See my examples/ directory to know how to do it concretely.). The class `Transform3f` can help you to place your objects arounf the world. See my examples.

* lines 66-67:
In this example, we defined matrices for the model view projection. The matrix `"projection"` is usually updated when the user changes the dimension of the GUI window (`onWindowSizeChanged(uint32 const width, uint32 const height)` of GLWindow and in `setup()`). The `"view"` matrix is updated when your camera is moved (see Legacy OpenGL `glLookAt` routine). Finally, the `"model"` matrix is used for placing models over the world (so to be placed before any `GLProgram::draw` methods).

**Note:** `Mode::TRIANGLES` is a strong type alias for `GL_TRIANGLES`. For all public methods needing an OpenGL GLint or GLenum types I replaced them by a C++ `enum class` (see GLEnum.hpp file for other types). This allows detecting bugs with the wrong type like trying to pass GL_DYNAMIC_DRAW instead of GL_TRIANGLES. Note that, inside private classes, I keep using GL_XXX names because they are not exposed to the public API.

**Note:** a GLSL coding rule suggests to prefix uniform names by "u_" and attribute names "a_". This is not mandatory for this API. The only requirement, for the moment, is to use the same name both for uniform and attribute is not managed by the API (and I'm not sure GLSL can be compiled anyway). To pass varying data between shader I personally use a structure. See my examples.

**Note:** Contrary to Glumpy, where GLProgram can modify both textures, uniforms, and attributes, in this API GLProgram only modifies uniforms and GLVAO modifies attributes (named VBO) or textures.

**Help wanted for a better C++ design allowing to shorten number of characters to type for setting attributes and uniforms**: C++ does not give the same flexibility than python for class typing. C++ is a strongly typed language (for classes). An example is C++ does not allow a container of template classes. Therefore in Glumpy, you will write `prog['position'] =` for setting either a shader attribute or a shader uniform, while with my API you cannot reach the same concise code. You'll have to explicit the template type and write `vao.VBO<Vector3f>("position") =` for attributes and `prog.uniform<float>("scale") =` for uniforms. `prog.uniform<Vector3f>("position") =` is 35 char sized for C++ while `prog['position'] =` is 18 char sized for Python (so 50% less). So use my methods to hide the C++ misery `scalarf(const char*)`, `vector3f(const char*)` ... Also, be careful that if you pass the wrong template type, an exception will be thrown telling you that the attribute/uniform name is known but the type is not available.

### Multiple objects: multiple VBOs vs. multiple VAOs

If you want to paint two cubes (let say for this example), you do not have to necessary create two VAOs (one by cube) but just a single VAO and do the following during the run-time process:

* Add [Model View Projection](http://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/) matrices in your vertex shader.
* Bind your VAO to your GLProgram.
* Use the Transformable class to place (translate, rotate, scale) your first cube in the scene: this will create a 4x4 Matrix.
* Apply this matrix to your shader uniform matrix named "model" of the Model View Projection.
* Draw the first cube.
* Use again the Transformable for placing the second cube: redo the three previous steps but this time, draw the second cube (update transformable, apply a matrix to uniform, draw).
* See my [examples](../../examples/README.md) directory to know how to do it concretely.

With this method, for this example, you will save GPU memory by avoiding storing an additional VAO and all its VBOs needed for drawing a cube.

### What else in OpenGLCppWrapper?

Not yet shown in tutorials but OpenGLCppWrapper can manage framebuffer and cube, 3D, 2D or 1D textures, multi-textures. To estimate the GPU memory usage. To make some security checks like detecting uninitilized VBOs, textures or VBO mismatch number of elements. To use Matrix and Vector classes (like glm library). To create transformable objects allowing to combine transformations like rotation, translation, projection on a model.

OpenGLCppWrapper includes other abstraction layers over this template code:
* to create a 'scene tree' of objects: a graph where nodes contain a transformation matrix relative to the current node and its parent.
* to wrap the initialization of [Dear ImGui](https://github.com/ocornut/imgui) an API for developing GUI in immediate mode (using just if then else instead of callbacks/signal-slot/observers methods like done with GTK+ or Qt).
* to wrap OpenGL window and offers to the user the init, run-time phase and callbacks like keyboard and windows resizing.

See my [examples](../../examples/README.md) directory to know how to do it concretely.

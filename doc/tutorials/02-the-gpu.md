# 2. The GPU

[Install and run](01-install.md) · [Tutorials](README.md) · [Next: the World](03-the-world.md)

![01c_Triangle](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/01c_Triangle.png)

The bottom layer of Compages talks to the graphics card. It knows shaders, buffers and textures, and nothing else: no objects, no camera, no scene. You will draw one triangle with a shader of your own, then make the GPU compute on a list of numbers.

## A triangle

A **shader** is a small program that runs on the GPU. The **vertex shader** places each corner of a shape. The **fragment shader** gives each pixel its colour. From [01c_Triangle.cpp](../../examples/01_FirstSteps/01c_Triangle.cpp):

```glsl
#version 450 core

in vec2 position;
in vec3 color;

out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
```

```glsl
#version 450 core

in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
```

The vertex shader asks for two inputs, `position` and `color`. The C++ code gives them by these names:

```cpp
compages::gpu::Drawable m_triangle;

COMPAGES_TRY(m_triangle.loadSources(VERTEX_SHADER, FRAGMENT_SHADER));
m_triangle["position"] = { { -0.8f, -0.6f }, { 0.8f, -0.6f }, { 0.0f, 0.8f } };
m_triangle["color"] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
return m_triangle.prepare();
```

At each frame:

```cpp
compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
m_triangle.draw();
```

The positions go from −1 to 1 across the window, so no camera is needed. `m_triangle["position"] = …` writes into a copy in normal memory. `draw()` sends what changed to the GPU, then draws. Your code has no OpenGL call and no attribute number to keep in mind.

A wrong name such as `m_triangle["colour"]` does not crash the program. Compages keeps the error, with the names the shader really declares, and `prepare()` returns it before the first frame. [Errors](../concepts/errors.md) explains when each kind of error comes back.

## A shader that computes

A **compute shader** draws nothing. The GPU runs it once per item, on many items at the same time. From [05a_Compute.cpp](../../examples/05_Simulation/05a_Compute.cpp), twelve numbers, each moved by its own index plus one:

```glsl
#version 450 core
layout(local_size_x = 16) in;

layout(std430, binding = 0) buffer Values
{
    int values[];
};

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= uint(values.length())) { return; }
    values[i] = (values[i] + int(i) + 1) % 60;
}
```

```cpp
COMPAGES_TRY(m_step.loadSource(STEP_SOURCE));
m_on_gpu = std::vector<int>(COUNT, 0);
COMPAGES_TRY_ASSIGN(m_values, compages::gpu::StorageBuffer<int>::from(m_on_gpu, compages::gpu::BufferUsage::Dynamic));

// at each step:
compages::gpu::dispatch(m_step, m_values);   // one copy of the shader per number
// when the panel asks:
m_values.download();                          // bring the numbers back to the CPU
```

The numbers live in a **storage buffer**: GPU memory that a shader can read and write. `dispatch()` runs the shader once per number. `download()` copies the result back to the CPU. It makes the CPU wait for the GPU. This is fine for twelve numbers in a lesson. For a million numbers at each frame, keep them on the GPU and draw them from there: `05f_Galaxy` moves eight thousand stars this way and never reads them back.

## Run them

```sh
./build/linux-gcc-debug/bin/Compages-examples 01c_Triangle
./build/linux-gcc-debug/bin/Compages-examples 05a_Compute
```

In `05a_Compute`, press **dispatch() once** in the panel. The GPU moves the numbers, and the bars turn orange because the copy on the CPU is now old. Press **download()** to bring the numbers back.

## Try

- In `01c_Triangle.cpp`, move the top corner to `{ 0.0f, 0.3f }`. The triangle gets flatter.
- Write `m_triangle["colour"]`. The console of the gallery lists the names the shader declares.
- In `05a_Compute`, replace `% 60` by `% 20`. The bars wrap sooner.

The Scene of the next pages uses this layer underneath, with shaders it writes for you. [The GPU layer](../concepts/the-gpu-layer.md) explains why the C++ and the shader meet by names. The guides [compute on the GPU](../guides/compute-on-the-gpu.md) and [draw in several passes](../guides/draw-in-several-passes.md) go further.

---

[Install and run](01-install.md) · [Tutorials](README.md) · [Next: the World](03-the-world.md)

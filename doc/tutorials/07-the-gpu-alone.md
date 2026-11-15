# 🔺 7. The GPU alone

[◀ What the robot sees](06-what-the-robot-sees.md) · [⏫ Tutorials](README.md)

![01c_Triangle](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/01c_Triangle.png)

The workshop used a Scene, which wrote the shaders for you. Sometimes you have no scene at all: a shader of your own and arrays of numbers, a picture computed pixel by pixel, a simulation of a million particles. You will draw one triangle with your own shader, then make the GPU compute on a list of numbers, with only the bottom layer of Compages.

## 🔺 A triangle named by its shader

From [01c_Triangle.cpp](../../examples/01_FirstSteps/01c_Triangle.cpp). The **vertex shader** places each corner, the **fragment shader** colours each pixel. The vertex shader declares what it needs by name:

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

The C++ side fills those names, and nothing else:

```cpp
compages::gpu::Drawable m_triangle;

COMPAGES_TRY(m_triangle.loadSources(VERTEX_SHADER, FRAGMENT_SHADER));
m_triangle["position"] = { { -0.8f, -0.6f }, { 0.8f, -0.6f }, { 0.0f, 0.8f } };
m_triangle["color"] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
return m_triangle.prepare();
```

and at each frame:

```cpp
compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
m_triangle.draw();
```

The positions are already screen coordinates (−1 to 1 across the picture): no camera here. `m_triangle["position"] = …` writes into a copy kept by the CPU; `draw()` sends what changed to the GPU, then draws. There is no OpenGL call in your code, no number to remember for each attribute: **the shader names, C++ fills, `draw()` sends.**

A wrong name (`m_triangle["colour"]`) does not crash. The error is kept, with the list of the names the shader really declares, and `prepare()` returns it before the first frame. [Errors](../concepts/errors.md) explains the two moments when Compages reports them.

## 🧮 A shader that computes

A **compute shader** draws nothing: the GPU runs it once per item, all at the same time. From [05a_Compute.cpp](../../examples/05_Simulation/05a_Compute.cpp), twelve numbers, each moved by its own index plus one:

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

The numbers live in a **storage buffer**, GPU memory the shader reads and writes. `dispatch()` runs the shader once per number and waits until what it wrote can be read. `download()` copies the result back to the CPU; it makes the CPU wait for the GPU, which is fine for twelve numbers in a lesson and wrong for a million at each frame. A real simulation keeps its data on the GPU and draws it from there: `05f_Galaxy` moves eight thousand stars this way, and never reads them back.

## ▶ Run them

```sh
./build/linux-gcc-debug/bin/Compages-examples 01c_Triangle
./build/linux-gcc-debug/bin/Compages-examples 05a_Compute
```

In `05a_Compute`, press **dispatch() once** in the panel: the GPU moves the numbers, and the bars turn orange, since the CPU copy is now late. Press **download()** to bring the numbers back.

## 🧪 Try

- In `01c_Triangle.cpp`, move the top corner to `{ 0.0f, 0.3f }`: a flatter triangle.
- Write `m_triangle["colour"]`: the console lists the names the shader declares.
- In `05a_Compute`, replace `% 60` by `% 20`: the bars wrap sooner.

## 💡 What happened

This layer, `compages::gpu`, is what the Scene uses underneath. It owns no scene, no entity, no camera: only shaders, buffers, textures, passes. You can use it alone, as here, or under a Scene for an effect of your own. [The GPU layer](../concepts/the-gpu-layer.md) explains the naming contract and who owns the memory; the guides [compute on the GPU](../guides/compute-on-the-gpu.md) and [draw in several passes](../guides/draw-in-several-passes.md) go further.

You have seen the whole of Compages. Go back to the [documentation map](../README.md) and choose a path.

---

[◀ What the robot sees](06-what-the-robot-sees.md) · [⏫ Tutorials](README.md)

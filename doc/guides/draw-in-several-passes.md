# 🖼 Draw in several passes

[⏫ Guides](README.md)

Draw into a texture and use the result in a later pass. Share one mesh between several shaders when the passes need the same geometry.

![04e_PostProcess](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/04e_PostProcess.png)

For filters of the picture of a Scene, [screen effects](screen-effects.md) do it all; this guide is for passes of your own, with the GPU layer.

## 🖼 Render once, read the texture afterward

Open [04d_RenderToTexture.cpp](../../examples/04_Passes/04d_RenderToTexture.cpp). Setup creates a 512 × 512 colour texture, a depth texture, and `m_target`, a framebuffer referring to them. It prepares `m_cube` for the scene and `m_blit` and `m_process` for full-screen draws. The latter two sample the target colour texture through their `image` uniform.

After setting the cube's matrices, its frame loop does:

```cpp
// Into the texture. The pass covers the whole framebuffer and suspends
// the window pass until the end of the scope.
{
    compages::gpu::RenderPass offscreen(
        m_target, { .color = { 0.07f, 0.07f, 0.1f, 1.0f } });
    m_cube.draw();
}

// Back on the window: the same texture, as it is on the left, treated on
// the right. The cube is never drawn here.
const std::uint32_t half = p_frame.width / 2u;
{
    compages::gpu::RenderPass left(
        { .width = half, .height = p_frame.height });
    m_blit.draw(3u);
}
{
    compages::gpu::RenderPass right({ .x = half,
                                      .width = p_frame.width - half,
                                      .height = p_frame.height });
    m_process.draw(3u);
}
```

The cube is drawn once into the texture. The two window passes read that picture: unchanged on the left, processed on the right. Finish the offscreen pass before sampling its texture, and never sample a texture that is attached for writing in the same draw. `p_frame` comes from the gallery and gives the window target's dimensions.

Run `./build/linux-gcc-debug/bin/Compages-examples 04d_RenderToTexture`. The processed side shifts the red and blue channels. Resize the window: the offscreen picture stays 512 × 512, while the two output viewports change size.

## 🧬 Reuse geometry across passes

[04c_MultiPassMesh.cpp](../../examples/04_Passes/04c_MultiPassMesh.cpp) keeps one `VertexBuffer<Vertex>` and three drawables that borrow it. Each shader reads the fields it needs. An undeclared field is skipped; a mismatched type is an error. Keep the borrowed buffer alive until those draws finish.

Below `Drawable`, use a `Program` and `Pipeline::create<Vertex>(program, state)` to check the struct against each shader, then draw the shared buffers. The [GPU layer concept](../concepts/the-gpu-layer.md#-who-owns-the-vertices) explains ownership and the tradeoff.

## 🎚 Passes and state

- A `RenderPass` chooses the target (a framebuffer, or the window), sets the viewport and clears. It ends at the closing brace of its scope; passes nest. `01b_ClearScreen` opens two passes, one per half of the window.
- `gpu::clear(color)` clears the current target.
- `drawable.state().depth_test = true` (blending, culling, primitive too) belongs to the drawable; the pipeline is rebuilt only when the state changes. `03c_DepthAndTransforms` uses the depth test and back-face culling.
- `gpu::forgetRenderState()`: after another library (ImGui) touched the context, the backend stops trusting its cached state.

A pass that draws **over a picture of a Scene** and tests its depth must use the Scene's reversed depth ([very large scenes](very-large-scenes.md#-reversed-depth)).

## 🧩 Share source between passes

Use [share and reload shader source](manage-shader-sources.md) when the passes need common GLSL functions, includes or hot reload. A source file's organisation does not determine where a pass draws.

## 🧯 Common problems

- Keep the sampled textures alive: a framebuffer refers to attachments and does not own their lifetime.
- Set the projection aspect ratio for the offscreen target, not the window.
- If another library changes OpenGL state, call `gpu::forgetRenderState()` before drawing through Compages again.
- For state updates that read neighbours, use [texture ping-pong](../techniques/texture-ping-pong.md).

## 🔗 See also

[◀ The GPU layer](../concepts/the-gpu-layer.md) · [Compute on the GPU](compute-on-the-gpu.md) · [Screen effects](screen-effects.md)

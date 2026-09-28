//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "50_Complete/DoomLike/Effects.hpp"
#include "50_Complete/DoomLike/Map.hpp"

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"

#include <algorithm>
#include <cmath>

namespace examples::doom
{

//! \brief A point is as large on the screen as \c size metres would be at
//! its distance; it fades toward its edge.
constexpr char const* VERTEX_SHADER = R"(#version 450 core
in vec3 position;
in vec4 color;
in float size;
uniform mat4 view;
uniform mat4 projection;
uniform float pixels;
out vec4 vColor;
void main()
{
    vec4 eye = view * vec4(position, 1.0);
    gl_Position = projection * eye;
    gl_PointSize = clamp(size * pixels / max(-eye.z, 0.05), 1.0, 256.0);
    vColor = color;
}
)";

constexpr char const* FRAGMENT_SHADER = R"(#version 450 core
in vec4 vColor;
out vec4 oColor;
void main()
{
    vec2 d = gl_PointCoord * 2.0 - 1.0;
    float r = dot(d, d);
    if (r > 1.0)
    {
        discard;
    }
    oColor = vec4(vColor.rgb, vColor.a * (1.0 - r));
}
)";

//! \brief The colours of the marks, as lit surfaces.
static Vector3f markColor(Mark p_mark)
{
    switch (p_mark)
    {
        case Mark::Blood: return Vector3f(0.32f, 0.01f, 0.01f);
        case Mark::Hole: return Vector3f(0.025f, 0.025f, 0.025f);
        case Mark::Scorch: return Vector3f(0.02f, 0.017f, 0.015f);
        case Mark::Oil: return Vector3f(0.03f, 0.03f, 0.035f);
    }
    return Vector3f(0.0f, 0.0f, 0.0f);
}

//! \brief The turn bringing +Z onto \c p_normal, then \c p_spin around it.
static Quatf facing(Vector3f p_normal, float p_spin)
{
    const Quatf spin = Quatf::fromAngleAxis(units::angle::radian_t(double(p_spin)),
                                            Vector3f(0.0f, 0.0f, 1.0f));
    const Vector3f z(0.0f, 0.0f, 1.0f);
    const float cosine = compages::vector::dot(z, p_normal);
    if (cosine > 0.9999f)
    {
        return spin;
    }
    if (cosine < -0.9999f)
    {
        return Quatf::fromAngleAxis(units::angle::radian_t(double(PI)),
                                    Vector3f(0.0f, 1.0f, 0.0f)) * spin;
    }
    Vector3f axis = compages::vector::cross(z, p_normal);
    axis.normalize();
    return Quatf::fromAngleAxis(units::angle::radian_t(double(std::acos(cosine))), axis) * spin;
}

//------------------------------------------------------------------------------
compages::gpu::Status Effects::setUp()
{
    // Depth tested so that walls hide them, depth not written so that they
    // do not hide one another.
    for (compages::gpu::Drawable* drawable : { &m_glowing, &m_hiding })
    {
        COMPAGES_TRY(drawable->load(VERTEX_SHADER, FRAGMENT_SHADER));
        drawable->primitive(compages::gpu::Primitive::Points).depthTest(true);
        drawable->state().depth_write = false;
    }
    m_glowing.blend(compages::gpu::Blend::additive());
    m_hiding.blend(compages::gpu::Blend::alpha());
    return compages::gpu::success();
}

//------------------------------------------------------------------------------
void Effects::reset(compages::renderer::Scene& p_scene)
{
    m_scene = &p_scene;
    m_particles.clear();
    m_tracers.clear();
    m_spreading.clear();
    for (auto& placed : m_placed)
    {
        placed.clear();
    }
    m_body_marks.clear();

    // A few splash shapes per kind of mark: a ragged disc and some drops
    // around it, flat, facing +Z. Far below the floor: only their copies show.
    for (std::size_t kind = 0u; kind < MARKS; ++kind)
    {
        compages::renderer::Look look = compages::renderer::color(0.0f, 0.0f, 0.0f);
        look.color = markColor(Mark(kind));
        for (std::size_t shape = 0u; shape < SHAPES; ++shape)
        {
            compages::renderer::MeshAsset mesh;
            auto add = [&mesh](float x, float y) {
                compages::renderer::MeshVertex vertex;
                vertex.position = Vector3f(x, y, 0.0f);
                vertex.normal = Vector3f(0.0f, 0.0f, 1.0f);
                vertex.uv = Vector2f(x, y);
                mesh.source_vertices.emplace_back(vertex);
                return std::uint32_t(mesh.source_vertices.size() - 1u);
            };
            auto disc = [&](float cx, float cy, float radius, std::size_t corners, float ragged) {
                const std::uint32_t middle = add(cx, cy);
                const std::uint32_t first = middle + 1u;
                for (std::size_t i = 0u; i < corners; ++i)
                {
                    const float angle = 2.0f * PI * float(i) / float(corners);
                    const float r = radius * random(1.0f - ragged, 1.0f);
                    (void)add(cx + (r * std::cos(angle)), cy + (r * std::sin(angle)));
                    const std::uint32_t next = std::uint32_t(first + ((i + 1u) % corners));
                    mesh.source_indices.emplace_back(middle);
                    mesh.source_indices.emplace_back(std::uint32_t(first + i));
                    mesh.source_indices.emplace_back(next);
                }
            };
            const bool round = (Mark(kind) == Mark::Hole);
            disc(0.0f, 0.0f, 0.5f, 18u, round ? 0.15f : 0.45f);
            const std::size_t drops = round ? 0u : 5u;
            for (std::size_t d = 0u; d < drops; ++d)
            {
                const float angle = random(0.0f, 2.0f * PI);
                const float at = random(0.55f, 0.8f);
                disc(at * std::cos(angle), at * std::sin(angle), random(0.04f, 0.1f), 7u, 0.3f);
            }
            mesh.index_count = mesh.source_indices.size();
            mesh.local_bounds = AABB::fromCorners(Vector3f(-0.9f, -0.9f, -0.01f),
                                                  Vector3f(0.9f, 0.9f, 0.01f));
            m_shapes[kind][shape] =
                p_scene.mesh(std::move(mesh), "MarkShape", look).position(0.0f, -50.0f, 0.0f).id();
        }
        m_drops[kind] = p_scene.sphere("DropShape", look).position(0.0f, -50.0f, 0.0f).id();
    }
}

//------------------------------------------------------------------------------
float Effects::random(float p_low, float p_high)
{
    return std::uniform_real_distribution<float>(p_low, p_high)(m_random);
}

//------------------------------------------------------------------------------
Vector3f Effects::randomDirection()
{
    Vector3f d(random(-1.0f, 1.0f), random(-1.0f, 1.0f), random(-1.0f, 1.0f));
    const float length = d.norm();
    return (length > 1.0e-3f) ? (d * (1.0f / length)) : Vector3f(0.0f, 1.0f, 0.0f);
}

//------------------------------------------------------------------------------
void Effects::emit(Particle p_particle)
{
    if (m_particles.size() < 4000u)
    {
        m_particles.emplace_back(p_particle);
    }
}

//------------------------------------------------------------------------------
void Effects::blood(Vector3f p_at, Vector3f p_direction, int p_count)
{
    for (int i = 0; i < p_count; ++i)
    {
        Particle drop;
        drop.position = p_at;
        drop.velocity = (p_direction * random(1.0f, 4.0f)) + (randomDirection() * 1.5f);
        const float dark = random(0.45f, 0.75f);
        drop.color = Vector3f(dark, 0.02f, 0.02f);
        drop.size = random(0.03f, 0.08f);
        drop.life = random(0.5f, 1.1f);
        drop.gravity = 9.0f;
        emit(drop);
    }
    // A red mist where it came out.
    Particle mist;
    mist.position = p_at;
    mist.velocity = p_direction * 0.4f;
    mist.color = Vector3f(0.5f, 0.02f, 0.02f);
    mist.size = 0.25f;
    mist.life = 0.35f;
    mist.grow = 0.8f;
    emit(mist);
}

//------------------------------------------------------------------------------
void Effects::sparks(Vector3f p_at, Vector3f p_normal, int p_count)
{
    for (int i = 0; i < p_count; ++i)
    {
        Particle spark;
        spark.position = p_at + (p_normal * 0.02f);
        spark.velocity = (p_normal * random(1.0f, 3.0f)) + (randomDirection() * 2.5f);
        spark.color = Vector3f(1.0f, random(0.55f, 0.8f), 0.25f);
        spark.size = random(0.02f, 0.04f);
        spark.life = random(0.15f, 0.4f);
        spark.gravity = 6.0f;
        spark.glows = true;
        emit(spark);
    }
    Particle dust;
    dust.position = p_at + (p_normal * 0.05f);
    dust.velocity = p_normal * 0.3f;
    dust.color = Vector3f(0.35f, 0.32f, 0.3f);
    dust.size = 0.15f;
    dust.life = 0.7f;
    dust.grow = 0.5f;
    dust.drag = 2.0f;
    emit(dust);
}

//------------------------------------------------------------------------------
void Effects::muzzle(Vector3f p_at, Vector3f p_direction)
{
    Particle flash;
    flash.position = p_at;
    flash.velocity = p_direction * 1.0f;
    flash.color = Vector3f(1.0f, 0.8f, 0.4f);
    flash.size = 0.22f;
    flash.life = 0.06f;
    flash.glows = true;
    emit(flash);
    for (int i = 0; i < 8; ++i)
    {
        Particle spark;
        spark.position = p_at;
        spark.velocity = (p_direction * random(4.0f, 9.0f)) + (randomDirection() * 1.2f);
        spark.color = Vector3f(1.0f, 0.7f, 0.3f);
        spark.size = 0.025f;
        spark.life = random(0.05f, 0.14f);
        spark.glows = true;
        emit(spark);
    }
    Particle smoke;
    smoke.position = p_at + (p_direction * 0.1f);
    smoke.velocity = (p_direction * 0.6f) + Vector3f(0.0f, 0.3f, 0.0f);
    smoke.color = Vector3f(0.5f, 0.5f, 0.5f);
    smoke.size = 0.08f;
    smoke.life = 0.8f;
    smoke.grow = 0.35f;
    smoke.drag = 1.5f;
    emit(smoke);
}

//------------------------------------------------------------------------------
void Effects::casing(Vector3f p_at, Vector3f p_right)
{
    Particle shell;
    shell.position = p_at;
    shell.velocity = (p_right * random(1.5f, 2.5f)) + Vector3f(0.0f, random(1.5f, 2.5f), 0.0f);
    shell.color = Vector3f(0.75f, 0.1f, 0.08f);
    shell.size = 0.03f;
    shell.life = 1.2f;
    shell.gravity = 9.8f;
    emit(shell);
}

//------------------------------------------------------------------------------
void Effects::explosion(Vector3f p_at)
{
    for (int i = 0; i < 70; ++i)
    {
        Particle fire;
        fire.position = p_at + (randomDirection() * 0.3f);
        fire.velocity = randomDirection() * random(1.0f, 5.0f) + Vector3f(0.0f, 1.5f, 0.0f);
        fire.color = Vector3f(1.0f, random(0.35f, 0.7f), 0.1f);
        fire.size = random(0.3f, 0.7f);
        fire.life = random(0.3f, 0.8f);
        fire.drag = 3.0f;
        fire.grow = 0.6f;
        fire.glows = true;
        emit(fire);
    }
    for (int i = 0; i < 60; ++i)
    {
        Particle spark;
        spark.position = p_at;
        spark.velocity = randomDirection() * random(5.0f, 12.0f);
        spark.color = Vector3f(1.0f, 0.8f, 0.4f);
        spark.size = 0.04f;
        spark.life = random(0.3f, 0.9f);
        spark.gravity = 8.0f;
        spark.glows = true;
        emit(spark);
    }
    for (int i = 0; i < 30; ++i)
    {
        Particle smoke;
        smoke.position = p_at + (randomDirection() * 0.5f);
        smoke.velocity = randomDirection() * 0.8f + Vector3f(0.0f, random(0.6f, 1.4f), 0.0f);
        const float grey = random(0.12f, 0.25f);
        smoke.color = Vector3f(grey, grey, grey);
        smoke.size = random(0.5f, 0.9f);
        smoke.life = random(1.5f, 3.0f);
        smoke.drag = 1.2f;
        smoke.grow = 0.7f;
        emit(smoke);
    }
}

//------------------------------------------------------------------------------
void Effects::glow(Vector3f p_at, Vector3f p_color, float p_size)
{
    Particle ball;
    ball.position = p_at;
    ball.velocity = Vector3f(0.0f, 0.0f, 0.0f);
    ball.color = p_color;
    ball.size = p_size;
    ball.life = 0.02f;
    ball.glows = true;
    emit(ball);
}

//------------------------------------------------------------------------------
void Effects::tracer(Vector3f p_from, Vector3f p_to, Vector3f p_color, float p_life)
{
    m_tracers.emplace_back(Tracer{ p_from, p_to, p_color, p_life });
}

//------------------------------------------------------------------------------
compages::world::Entity Effects::takeMark(Mark p_mark)
{
    // A new copy until enough are around, then the oldest moves.
    const std::size_t kind = std::size_t(p_mark);
    std::deque<compages::world::Entity>& placed = m_placed[kind];
    if (placed.size() >= MARKS_KEPT)
    {
        compages::world::Entity oldest = placed.front();
        placed.pop_front();
        placed.emplace_back(oldest);
        return oldest;
    }
    const std::size_t shape = std::size_t(random(0.0f, float(SHAPES) - 0.01f));
    placed.emplace_back(m_scene->copy(m_shapes[kind][shape], "Mark"));
    return placed.back();
}

//------------------------------------------------------------------------------
void Effects::mark(Mark p_mark, Vector3f p_point, Vector3f p_normal, float p_size)
{
    if (m_scene == nullptr)
    {
        return;
    }
    // Just off the surface, so that the two do not fight for the same depth;
    // a little further for each new one, so that marks over marks show.
    static float lift = 0.0f;
    lift = (lift > 0.01f) ? 0.0f : (lift + 0.0007f);
    takeMark(p_mark)
        .position(p_point + (p_normal * (0.008f + lift)))
        .rotation(facing(p_normal, random(0.0f, 2.0f * PI)))
        .scale(p_size);
}

//------------------------------------------------------------------------------
void Effects::pool(Mark p_mark, Vector3f p_at, float p_size)
{
    if (m_scene == nullptr)
    {
        return;
    }
    compages::world::Entity puddle = takeMark(p_mark);
    puddle.position(p_at.x, 0.012f, p_at.z)
        .rotation(facing(Vector3f(0.0f, 1.0f, 0.0f), random(0.0f, 2.0f * PI)))
        .scale(0.05f);
    m_spreading.emplace_back(Spread{ puddle, 0.05f, p_size });
}

//------------------------------------------------------------------------------
void Effects::markBody(Mark p_mark, compages::world::EntityId p_body, Vector3f p_point, float p_size)
{
    if (m_scene == nullptr)
    {
        return;
    }
    compages::world::World& world = m_scene->world();

    // The bone nearest to the hit: an entity of the model drawing nothing.
    compages::world::EntityId nearest;
    float best = 1.0e9f;
    std::vector<compages::world::EntityId> open{ world.firstChild(p_body) };
    while (!open.empty())
    {
        const compages::world::EntityId id = open.back();
        open.pop_back();
        if (!id.valid())
        {
            continue;
        }
        open.emplace_back(world.nextSibling(id));
        open.emplace_back(world.firstChild(id));
        if (world.has<compages::renderer::MeshRenderer>(id))
        {
            continue;
        }
        Matrix44f const& m = world.worldMatrix(id);
        const Vector3f at(m[3].x, m[3].y, m[3].z);
        const float d = (at - p_point).norm();
        if (d < best)
        {
            best = d;
            nearest = id;
        }
    }
    if (!nearest.valid())
    {
        return;
    }

    // In the space of the bone: the point brought back through its matrix,
    // the size divided by its scale, which in a model made in centimetres is
    // a hundredth.
    const Matrix44f inverse = compages::matrix::inverse(world.worldMatrix(nearest));
    const Vector3f local(
        (p_point.x * inverse[0].x) + (p_point.y * inverse[1].x) + (p_point.z * inverse[2].x) + inverse[3].x,
        (p_point.x * inverse[0].y) + (p_point.y * inverse[1].y) + (p_point.z * inverse[2].y) + inverse[3].y,
        (p_point.x * inverse[0].z) + (p_point.y * inverse[1].z) + (p_point.z * inverse[2].z) + inverse[3].z);
    Matrix44f const& m = world.worldMatrix(nearest);
    const float scale = std::max(Vector3f(m[0].x, m[0].y, m[0].z).norm(), 1.0e-6f);

    compages::world::Entity drop;
    if (m_body_marks.size() >= MARKS_KEPT)
    {
        drop = m_body_marks.front();
        m_body_marks.pop_front();
    }
    else
    {
        drop = m_scene->copy(m_drops[std::size_t(p_mark)], "BodyMark");
    }
    m_body_marks.emplace_back(drop);
    drop.parent(compages::world::Entity(world, nearest)).position(local).scale(p_size / scale);
}

//------------------------------------------------------------------------------
void Effects::update(float p_dt)
{
    for (Particle& p : m_particles)
    {
        p.age += p_dt;
        p.velocity.y -= p.gravity * p_dt;
        p.velocity = p.velocity * std::max(0.0f, 1.0f - (p.drag * p_dt));
        p.position = p.position + (p.velocity * p_dt);
        p.size += p.grow * p_dt;
        // What falls lands, and stays a moment.
        if ((p.gravity > 0.0f) && (p.position.y < 0.01f))
        {
            p.position.y = 0.01f;
            p.velocity = Vector3f(p.velocity.x * 0.3f, -p.velocity.y * 0.2f, p.velocity.z * 0.3f);
        }
    }
    std::erase_if(m_particles, [](Particle const& p) { return p.age >= p.life; });

    for (Tracer& t : m_tracers)
    {
        t.age += p_dt;
    }
    std::erase_if(m_tracers, [](Tracer const& t) { return t.age >= t.life; });

    for (Spread& s : m_spreading)
    {
        s.size = std::min(s.target, s.size + (p_dt * 0.5f));
        s.entity.scale(s.size);
    }
    std::erase_if(m_spreading, [](Spread const& s) { return s.size >= s.target; });
}

//------------------------------------------------------------------------------
void Effects::lines(compages::renderer::DebugDraw& p_debug) const
{
    for (Tracer const& t : m_tracers)
    {
        const float fade = 1.0f - (t.age / t.life);
        p_debug.line(t.from, t.to, t.color * fade);
    }
}

//------------------------------------------------------------------------------
void Effects::draw(compages::renderer::CameraFrame const& p_camera, float p_fov_degrees, float p_height)
{
    // Rebuilt each frame: a few hundred points, cheaper to send again than to
    // keep track of.
    m_glowing.clear();
    m_hiding.clear();
    for (Particle const& p : m_particles)
    {
        const float t = p.age / p.life;
        float alpha = 1.0f - t;
        if (p.grow > 0.0f)
        {
            // What swells, smoke and mist, comes in before it fades out.
            alpha *= std::min(1.0f, t * 6.0f) * 0.7f;
        }
        const Point point{ p.position, Vector4f(p.color.x, p.color.y, p.color.z, alpha), p.size };
        (p.glows ? m_glowing : m_hiding).emplace_back(point);
    }

    const float pixels = p_height * 0.5f / std::tan(p_fov_degrees * PI / 360.0f);
    for (compages::gpu::Drawable* drawable : { &m_hiding, &m_glowing })
    {
        if (drawable->count() == 0u)
        {
            continue;
        }
        (*drawable)["view"] = p_camera.view;
        (*drawable)["projection"] = p_camera.projection;
        (*drawable)["pixels"] = pixels;
        drawable->draw();
    }
}

} // namespace examples::doom

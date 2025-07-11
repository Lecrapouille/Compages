//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "main.hpp"
#define protected public
#define private public
#include "OpenGL/Buffers/VAO.hpp"
#include "OpenGL/Shaders/Program.hpp"
#undef protected
#undef private

// Check initial states
TEST(TestGLVAO, TestCreators)
{
    // No OpenGL context
    {
        GLVAO vao("vao");

        // Check GLObject states
        ASSERT_STREQ("vao", vao.cname());
        ASSERT_EQ(0, vao.m_handle);
        ASSERT_EQ(GL_ARRAY_BUFFER, vao.m_target); // FIXME
        ASSERT_EQ(true, vao.m_need_setup);
        ASSERT_EQ(true, vao.m_need_create);
        ASSERT_EQ(false, vao.m_need_update);

        // Check GLVAO states
        ASSERT_EQ(0_z, vao.m_vbos.size());
        ASSERT_EQ(0_z, vao.m_textures.size());
        ASSERT_EQ(nullptr, vao.m_program);
        ASSERT_EQ(0_z, vao.m_count);
        ASSERT_EQ(3_z, vao.m_reserve);
        ASSERT_EQ(BufferUsage::DYNAMIC_DRAW, vao.m_usage);
        ASSERT_EQ(false, vao.isBound());
    }

    // With OpenGL context
    OpenGLContext context(
        []()
        {
            GLVAO vao("vao");

            // Check GLObject states
            ASSERT_STREQ("vao", vao.cname());
            ASSERT_EQ(0, vao.m_handle);
            ASSERT_EQ(GL_ARRAY_BUFFER, vao.m_target);
            ASSERT_EQ(true, vao.m_need_setup);
            ASSERT_EQ(true, vao.m_need_create);
            ASSERT_EQ(false, vao.m_need_update);

            // Check GLVAO states
            ASSERT_EQ(0_z, vao.m_vbos.size());
            ASSERT_EQ(0_z, vao.m_textures.size());
            ASSERT_EQ(nullptr, vao.m_program);
            ASSERT_EQ(0_z, vao.m_count);
            ASSERT_EQ(3_z, vao.m_reserve);
            ASSERT_EQ(BufferUsage::DYNAMIC_DRAW, vao.m_usage);
            ASSERT_EQ(false, vao.isBound());
        });
}

TEST(TestGLVAO, TestBindUncompiledProg)
{
    // With OpenGL context
    OpenGLContext context(
        []()
        {
            GLVertexShader vs;
            GLFragmentShader fs;
            GLProgram prog("prog");
            GLVAO vao("vao");

            ASSERT_EQ(false, prog.compiled());
            ASSERT_EQ(false, prog.bind(vao));
            EXPECT_THAT(prog.strerror().c_str(), HasSubstr("bind GLVAO"));

            // Check GLObject states
            ASSERT_STREQ("vao", vao.cname());
            ASSERT_EQ(0, vao.m_handle);
            ASSERT_EQ(GL_ARRAY_BUFFER, vao.m_target);
            ASSERT_EQ(true, vao.m_need_setup);
            ASSERT_EQ(true, vao.m_need_create);
            ASSERT_EQ(false, vao.m_need_update);

            // Check GLVAO states
            ASSERT_EQ(0_z, vao.m_vbos.size());
            ASSERT_EQ(0_z, vao.m_textures.size());
            ASSERT_EQ(nullptr, vao.m_program);
            ASSERT_EQ(0_z, vao.m_count);
            ASSERT_EQ(3_z, vao.m_reserve);
            ASSERT_EQ(BufferUsage::DYNAMIC_DRAW, vao.m_usage);
            ASSERT_EQ(false, vao.isBound());
        });
}

TEST(TestGLVAO, TestNominalBind)
{
    // With OpenGL context
    OpenGLContext context(
        []()
        {
            std::vector<std::string> names;
            GLVertexShader vs;
            GLFragmentShader fs;
            GLProgram prog("prog");

            // Pathes for finding shaders
            vs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");
            fs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");

            // Load GLSL codes
            ASSERT_EQ(true, vs.read("test4.vs"));
            ASSERT_EQ(true, fs.read("test4.fs"));
            ASSERT_EQ(true, prog.compile(vs, fs));
            ASSERT_EQ(true, prog.compiled());

            GLVAO vao("vao");
            ASSERT_EQ(true, prog.bind(vao));
            ASSERT_EQ(true, vao.isBound());

            // Check GLObject states
            ASSERT_STREQ("vao", vao.cname());
            ASSERT_EQ(0, vao.m_handle);
            ASSERT_EQ(GL_ARRAY_BUFFER, vao.m_target);
            ASSERT_EQ(true, vao.m_need_setup);
            ASSERT_EQ(true, vao.m_need_create);
            ASSERT_EQ(true, vao.m_need_update);

            // Check GLVAO states
            ASSERT_EQ(&prog, vao.m_program);
            ASSERT_EQ(0_z, vao.m_count);
            ASSERT_EQ(3_z, vao.m_reserve);
            ASSERT_EQ(BufferUsage::DYNAMIC_DRAW, vao.m_usage);
            ASSERT_EQ(0_z, vao.m_textures.size());
            ASSERT_EQ(0_z, vao.hasTextures());
            ASSERT_EQ(0_z, vao.getTexturesNames(names));
            ASSERT_EQ(0_z, names.size());
            ASSERT_EQ(2_z, vao.hasVBOs());
            ASSERT_EQ(2_z, vao.m_vbos.size());
            ASSERT_EQ(true, vao.hasVBO<Vector2f>("position"));
            ASSERT_EQ(false, vao.hasVBO<Vector2f>("foobar"));
            ASSERT_EQ(true, vao.hasVBO<Vector3f>("color"));
            ASSERT_EQ(false, vao.hasVBO<Vector4f>("color"));
            ASSERT_EQ(2_z, vao.getVBONames(names));
            ASSERT_EQ(2_z, names.size());
            EXPECT_THAT(names, UnorderedElementsAre("position", "color"));

            // Check GLVAO's VBO states
            GLVertexBuffer<Vector2f>& pos = vao.vector2f("position");
            ASSERT_STREQ("position", pos.cname());
            ASSERT_EQ(0, pos.m_handle);
            ASSERT_EQ(GL_ARRAY_BUFFER, pos.m_target);
            ASSERT_EQ(true, pos.m_need_setup);
            ASSERT_EQ(true, pos.m_need_create);
            ASSERT_EQ(false, pos.m_need_update);
            ASSERT_EQ(GL_DYNAMIC_DRAW, pos.m_usage);
            ASSERT_EQ(0_z, pos.m_container.size());
            ASSERT_EQ(0_z, pos.size());
            ASSERT_EQ(false, pos.isPending());
            ASSERT_EQ(static_cast<size_t>(-1),
                      pos.m_pending_start); // FIXME Pending::npos
            ASSERT_EQ(static_cast<size_t>(-1), pos.m_pending_end);

            // Check GLVAO's VBO states
            GLVertexBuffer<Vector3f>& color = vao.vector3f("color");
            ASSERT_STREQ("color", color.cname());
            ASSERT_EQ(0, color.m_handle);
            ASSERT_EQ(GL_ARRAY_BUFFER, color.m_target);
            ASSERT_EQ(true, color.m_need_setup);
            ASSERT_EQ(true, color.m_need_create);
            ASSERT_EQ(false, color.m_need_update);
            ASSERT_EQ(GL_DYNAMIC_DRAW, color.m_usage);
            ASSERT_EQ(0_z, color.m_container.size());
            ASSERT_EQ(0_z, color.size());
            ASSERT_EQ(false, color.isPending());
            ASSERT_EQ(static_cast<size_t>(-1),
                      color.m_pending_start); // FIXME Pending::ncolor
            ASSERT_EQ(static_cast<size_t>(-1), color.m_pending_end);

            // Set values to VBOs
            pos = { Vector2f(1.0f, 2.0f),
                    Vector2f(2.0f, 3.0f),
                    Vector2f(4.0f, 5.0f) };
            ASSERT_EQ(true, pos.m_need_setup);
            ASSERT_EQ(true, pos.m_need_create);
            std::cout << "QQQQBuffer::needUpdate() " << std::endl;
            ASSERT_EQ(true, pos.needUpdate());
            ASSERT_EQ(true, pos.m_need_update); // FIXME: virer needUpdate()
            ASSERT_EQ(GL_DYNAMIC_DRAW, pos.m_usage);
            ASSERT_EQ(3_z, pos.m_container.size());
            ASSERT_EQ(3_z, pos.size());
            ASSERT_EQ(true, pos.isPending());
            ASSERT_EQ(0_z, pos.m_pending_start); // FIXME Pending::npos
            ASSERT_EQ(3_z, pos.m_pending_end);

            // Set values to VBOs
            color = { Vector3f(1.0f, 2.0f, 3.0f) };
            ASSERT_EQ(true, color.m_need_setup);
            ASSERT_EQ(true, color.m_need_create);
            // ASSERT_EQ(true, color.m_need_update);
            ASSERT_EQ(GL_DYNAMIC_DRAW, color.m_usage);
            ASSERT_EQ(1_z, color.m_container.size());
            ASSERT_EQ(1_z, color.size());
            ASSERT_EQ(true, pos.isPending());
            ASSERT_EQ(0_z, color.m_pending_start); // FIXME Pending::ncolor
            ASSERT_EQ(1_z, color.m_pending_end);
        });
}

// VBO + modif apres bind

// VBO + modif avant bind

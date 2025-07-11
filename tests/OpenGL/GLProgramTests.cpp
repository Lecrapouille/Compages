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
TEST(TestGLPrograms, TestCreators)
{
    // No OpenGL context
    {
        std::vector<std::string> list;

        GLProgram prog("prog");

        // Check GLObject states
        ASSERT_STREQ("prog", prog.cname());
        ASSERT_EQ(0, prog.m_handle);
        ASSERT_EQ(0, prog.m_target);
        ASSERT_EQ(true, prog.m_need_setup);
        ASSERT_EQ(true, prog.m_need_create);
        ASSERT_EQ(false, prog.m_need_update);

        // Check GLProgram states
        ASSERT_EQ(0_z, prog.m_shaders.size());
        ASSERT_EQ(false, prog.compiled());
        ASSERT_EQ(0_z, prog.getFailedShaders(list, true));
        ASSERT_EQ(0_z, list.size());
        ASSERT_EQ(0_z, prog.getUniformNames(list, true));
        ASSERT_EQ(0_z, list.size());
        ASSERT_EQ(0_z, prog.getAttributeNames(list, true));
        ASSERT_EQ(0_z, list.size());
        ASSERT_EQ(0_z, prog.getSamplerNames(list, true));
        ASSERT_EQ(0_z, list.size());
        ASSERT_EQ(0_z, prog.m_attributes.size());
        ASSERT_EQ(0_z, prog.m_uniforms.size());
        ASSERT_EQ(0_z, prog.m_samplers.size());
        ASSERT_EQ(0_z, prog.attributes().size());
        ASSERT_EQ(0_z, prog.uniforms().size());
        ASSERT_EQ(0_z, prog.samplers().size());
        ASSERT_EQ(0_z, prog.m_failedShaders.size());
        ASSERT_STREQ("", prog.m_error.c_str());
        ASSERT_STREQ("", prog.strerror().c_str());
    }

    // With OpenGL context
    OpenGLContext context(
        []()
        {
            std::vector<std::string> list;

            GLProgram prog("prog");

            // Check GLObject states
            ASSERT_STREQ("prog", prog.cname());
            ASSERT_EQ(0, prog.m_handle);
            ASSERT_EQ(0, prog.m_target);
            ASSERT_EQ(true, prog.m_need_setup);
            ASSERT_EQ(true, prog.m_need_create);
            ASSERT_EQ(false, prog.m_need_update);

            // Check GLProgram states
            ASSERT_EQ(0_z, prog.m_shaders.size());
            ASSERT_EQ(false, prog.compiled());
            ASSERT_EQ(0_z, prog.getFailedShaders(list, true));
            ASSERT_EQ(0_z, list.size());
            ASSERT_EQ(0_z, prog.getUniformNames(list, true));
            ASSERT_EQ(0_z, list.size());
            ASSERT_EQ(0_z, prog.getAttributeNames(list, true));
            ASSERT_EQ(0_z, list.size());
            ASSERT_EQ(0_z, prog.getSamplerNames(list, true));
            ASSERT_EQ(0_z, list.size());
            ASSERT_EQ(0_z, prog.m_attributes.size());
            ASSERT_EQ(0_z, prog.m_uniforms.size());
            ASSERT_EQ(0_z, prog.m_samplers.size());
            ASSERT_EQ(0_z, prog.attributes().size());
            ASSERT_EQ(0_z, prog.uniforms().size());
            ASSERT_EQ(0_z, prog.samplers().size());
            ASSERT_EQ(0_z, prog.m_failedShaders.size());
            ASSERT_STREQ("", prog.m_error.c_str());
            ASSERT_STREQ("", prog.strerror().c_str());
        });
}

// Test we cannot compile dummy code
TEST(TestGLPrograms, TestDummyShaderCompilation)
{
    // No OpenGL context
    {
        std::vector<std::string> list;

        GLProgram prog("prog");
        prog.begin(); // GLObject::begin()

        ASSERT_EQ(false, prog.compiled());
        ASSERT_EQ(0, prog.m_handle);
        ASSERT_EQ(0, prog.m_target);
        ASSERT_EQ(true, prog.m_need_setup);
        ASSERT_EQ(true, prog.m_need_create);
        ASSERT_EQ(false, prog.m_need_update);
        ASSERT_EQ(0_z, prog.getFailedShaders(list, true));
        ASSERT_EQ(0_z, list.size());
    }

    // With OpenGL context
    OpenGLContext context(
        []()
        {
            std::vector<std::string> list;

            GLProgram prog("prog");
            prog.begin(); // GLObject::begin()

            ASSERT_EQ(false, prog.compiled());
            ASSERT_EQ(1, prog.m_handle);
            ASSERT_EQ(0, prog.m_target);
            ASSERT_EQ(true, prog.m_need_setup);
            ASSERT_EQ(false, prog.m_need_create);
            ASSERT_EQ(false, prog.m_need_update);
            ASSERT_EQ(0_z, prog.getFailedShaders(list, true));
            ASSERT_EQ(0_z, list.size());
        });
}

// Test that we cannot compile dummy shader codes
TEST(TestGLPrograms, TestCompilationDummyShaders)
{
    OpenGLContext context(
        []()
        {
            std::vector<std::string> list;

            // Empty shader code
            GLVertexShader vertex("vs");
            GLFragmentShader fragment("fs");

            // Check dummy GLSL code
            ASSERT_STREQ("", vertex.m_code.c_str());
            ASSERT_EQ(false, vertex.loaded());
            ASSERT_STREQ("", fragment.m_code.c_str());
            ASSERT_EQ(false, fragment.loaded());

            // Compile shaders
            GLProgram prog("prog");
            ASSERT_EQ(false, prog.compile(vertex, fragment));
            ASSERT_EQ(false, prog.compiled());
            ASSERT_EQ(1, prog.m_handle); // onCreated() was called
            ASSERT_EQ(0, prog.m_target);
            ASSERT_EQ(true, prog.m_need_setup);
            ASSERT_EQ(false, prog.m_need_create); // onCreated() was called
            ASSERT_EQ(false, prog.m_need_update);
            ASSERT_EQ(2_z, prog.getFailedShaders(list, true));
            ASSERT_EQ(2_z, list.size());
            ASSERT_STREQ("vs", list[0].c_str());
            ASSERT_STREQ("fs", list[1].c_str());
            ASSERT_EQ(false, prog.strerror().empty()); // errored
        });
}

// Test we can create uniform if the prog is not compiled
TEST(TestGLPrograms, TestCreatUniformProgNotCompiled)
{
    OpenGLContext context(
        []()
        {
            std::vector<std::string> list;

            GLProgram prog("prog");

            // Check uniforms creation
            prog.createUniform<float>("u1");
            prog.createUniform<Vector2f>("u1"); // double name with dif type
            prog.createUniform<Vector2f>("u2");
            prog.createUniform<Vector3f>("u3");
            prog.createUniform<Vector4f>("u4");
            ASSERT_EQ(4_z, prog.getUniformNames(list, true)); // instead of 5
            ASSERT_EQ(list.size(), 4u);
            ASSERT_THAT(
                list,
                ElementsAre("u1", "u2", "u3", "u4")); // "u1" not in double
            ASSERT_EQ(false, prog.hasUniform<float>("u1"));
            ASSERT_EQ(true, prog.hasUniform<Vector2f>("u1"));
            ASSERT_EQ(true, prog.hasUniform<Vector2f>("u1"));
            ASSERT_EQ(true, prog.hasUniform<Vector2f>("u2"));
            ASSERT_EQ(true, prog.hasUniform<Vector3f>("u3"));
            ASSERT_EQ(true, prog.hasUniform<Vector4f>("u4"));

            // Check bad accessor
            try
            {
                prog.uniform<int>(nullptr);
                FAIL() << "Exception should have occured";
            }
            catch (...)
            {
            }

            // Check accessor with known names and correct type
            try
            {
                prog.uniform<float>("u1");
            }
            catch (...)
            {
                FAIL() << "Exception should not have occured";
            }
            try
            {
                prog.uniform<Vector2f>("u1");
            }
            catch (...)
            {
                FAIL() << "Exception should not have occured";
            }
            try
            {
                prog.uniform<Vector2f>("u2");
            }
            catch (...)
            {
                FAIL() << "Exception should not have occured";
            }
            try
            {
                prog.uniform<Vector3f>("u3");
            }
            catch (...)
            {
                FAIL() << "Exception should not have occured";
            }
            try
            {
                prog.uniform<Vector4f>("u4");
            }
            catch (...)
            {
                FAIL() << "Exception should not have occured";
            }

            // Program not yet compiled: accessing to unknown uniforms
            // make create them.
            try
            {
                prog.uniform<int>("u5");
            }
            catch (...)
            {
                FAIL() << "Exception should not have occured";
            }
            ASSERT_EQ(5_z, prog.getUniformNames(list, true));
            ASSERT_EQ(list.size(), 5u);
            ASSERT_THAT(list, ElementsAre("u1", "u2", "u3", "u4", "u5"));

            // Collision name
            try
            {
                prog.uniform<float>("u5");
            }
            catch (...)
            {
                FAIL() << "Exception should not have occured";
            }
            ASSERT_EQ(5_z, prog.getUniformNames(list, true));
            ASSERT_EQ(list.size(), 5u);
            ASSERT_THAT(list, ElementsAre("u1", "u2", "u3", "u4", "u5"));

            // Program compiled: accessing to unknown uniforms is no
            // longer allowed
            ASSERT_EQ(false, prog.compiled());
            prog.m_need_setup = false;
            ASSERT_EQ(true, prog.compiled());

            // Check accessor with known names but wrong type
            try
            {
                prog.uniform<int>("u4");
                FAIL() << "Exception should have occured";
            }
            catch (...)
            {
            }
            ASSERT_EQ(5_z, prog.getUniformNames(list, true));
            ASSERT_EQ(list.size(), 5u);
            ASSERT_THAT(list, ElementsAre("u1", "u2", "u3", "u4", "u5"));
        });
}

// Test nominal compilation of shaders
TEST(TestGLPrograms, testSuccessCompilation1)
{
    OpenGLContext context(
        []()
        {
            std::vector<std::string> names;
            size_t count;

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

            // Compile and check compilation done with success
            ASSERT_EQ(true, prog.compile(vs, fs));
            ASSERT_EQ(true, vs.compiled());
            ASSERT_STREQ("", vs.strerror().c_str());
            ASSERT_EQ(true, fs.compiled());
            ASSERT_STREQ("", vs.strerror().c_str());
            ASSERT_STREQ("", prog.strerror().c_str());
            ASSERT_EQ(true, prog.compiled());

            // Check that shaders have been removed
            count = prog.m_shaders.size();
            ASSERT_EQ(0_z, count);

            // Check getter of failed compiled shader is empty
            count = prog.getFailedShaders(names);
            ASSERT_EQ(0_z, count);
            ASSERT_EQ(0_z, names.size());
            ASSERT_EQ(0_z, prog.m_failedShaders.size());

            // Get sampler variable names. Here none are used.
            count = prog.getSamplerNames(names);
            ASSERT_EQ(0_z, count);
            ASSERT_EQ(0_z, names.size());
            ASSERT_EQ(0_z, prog.m_samplers.size());

            // Get attribute variable names. Here none are used.
            count = prog.getAttributeNames(names);
            ASSERT_EQ(2_z, count);
            ASSERT_EQ(2_z, names.size());
            EXPECT_THAT(names, UnorderedElementsAre("position", "color"));
            ASSERT_EQ(2_z, prog.m_attributes.size());
            names.clear();
            for (auto const& it : prog.m_attributes)
                names.push_back(it.first);
            EXPECT_THAT(names, UnorderedElementsAre("position", "color"));

            // Get uniform variable names. Here none are used.
            count = prog.getUniformNames(names);
            ASSERT_EQ(0_z, count);
            ASSERT_EQ(0_z, names.size());
            ASSERT_EQ(0_z, prog.uniforms().size());

            // Check GLObject states
            ASSERT_STREQ("prog", prog.cname());
            ASSERT_EQ(1, prog.m_handle);
            ASSERT_EQ(0, prog.m_target);
            ASSERT_EQ(false, prog.m_need_setup);
            ASSERT_EQ(false, prog.m_need_create);
            ASSERT_EQ(true, prog.m_need_update);

            // Check attributes states
            GLAttribute& pos = *(prog.m_attributes["position"]);
            ASSERT_STREQ("position", pos.cname());
            ASSERT_EQ(-1, pos.m_handle);
            ASSERT_EQ(GL_FLOAT, pos.m_target);
            ASSERT_EQ(2, pos.m_size);
            ASSERT_EQ(prog.m_handle, pos.m_program);
            ASSERT_EQ(0, pos.m_index);
            ASSERT_EQ(0, pos.m_stride);
            ASSERT_EQ(0, pos.m_offset);
            ASSERT_EQ(true, pos.m_need_setup);
            ASSERT_EQ(true, pos.m_need_create);
            ASSERT_EQ(false, pos.m_need_update);

            GLAttribute& color = *(prog.m_attributes["color"]);
            ASSERT_STREQ("color", color.cname());
            ASSERT_EQ(-1, color.m_handle);
            ASSERT_EQ(GL_FLOAT, color.m_target);
            ASSERT_EQ(3, color.m_size);
            ASSERT_EQ(prog.m_handle, color.m_program);
            ASSERT_EQ(0, color.m_index);
            ASSERT_EQ(0, color.m_stride);
            ASSERT_EQ(0, color.m_offset);
            ASSERT_EQ(true, color.m_need_setup);
            ASSERT_EQ(true, color.m_need_create);
            ASSERT_EQ(false, color.m_need_update);
        });
}

// Test nominal compilation of shaders
TEST(TestGLPrograms, testSuccessCompilation2)
{
    OpenGLContext context(
        []()
        {
            std::vector<std::string> names;
            size_t count;

            GLVertexShader vs;
            GLFragmentShader fs;
            GLProgram prog("prog");

            // Pathes for finding shaders
            vs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");
            fs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");

            // Load GLSL codes
            ASSERT_EQ(true, vs.read("test5.vs"));
            ASSERT_EQ(true, fs.read("test5.fs"));

            // Compile and check compilation done with success
            ASSERT_EQ(true, prog.compile(vs, fs));
            ASSERT_EQ(true, vs.compiled());
            ASSERT_STREQ("", vs.strerror().c_str());
            ASSERT_EQ(true, fs.compiled());
            ASSERT_STREQ("", vs.strerror().c_str());
            ASSERT_STREQ("", prog.strerror().c_str());
            ASSERT_EQ(true, prog.compiled());

            // Check that shaders have been removed
            count = prog.m_shaders.size();
            // ASSERT_EQ(0_z, count);

            // Check getter of failed compiled shader is empty
            count = prog.getFailedShaders(names);
            ASSERT_EQ(0_z, count);
            ASSERT_EQ(0_z, names.size());
            ASSERT_EQ(0_z, prog.m_failedShaders.size());

            // Get sampler variable names. Here none are used.
            count = prog.getSamplerNames(names);
            ASSERT_EQ(0_z, count);
            ASSERT_EQ(0_z, names.size());
            ASSERT_EQ(0_z, prog.m_samplers.size());

            // Get attribute variable names. Here none are used.
            count = prog.getAttributeNames(names);
            ASSERT_EQ(1_z, count);
            ASSERT_EQ(1_z, names.size());
            EXPECT_THAT(names, UnorderedElementsAre("aPos"));
            ASSERT_EQ(1_z, prog.m_attributes.size());
            names.clear();
            for (auto const& it : prog.m_attributes)
                names.push_back(it.first);
            EXPECT_THAT(names, UnorderedElementsAre("aPos"));

            // Get uniform variable names. Here none are used.
            count = prog.getUniformNames(names);
            ASSERT_EQ(1_z, count);
            ASSERT_EQ(1_z, names.size());
            EXPECT_THAT(names, UnorderedElementsAre("ourColor"));
            ASSERT_EQ(1_z, prog.m_uniforms.size());
            names.clear();
            for (auto const& it : prog.m_uniforms)
                names.push_back(it.first);
            EXPECT_THAT(names, UnorderedElementsAre("ourColor"));

            // Check GLObject states
            ASSERT_STREQ("prog", prog.cname());
            ASSERT_EQ(1, prog.m_handle);
            ASSERT_EQ(0, prog.m_target);
            ASSERT_EQ(false, prog.m_need_setup);
            ASSERT_EQ(false, prog.m_need_create);
            ASSERT_EQ(true, prog.m_need_update);

            // Check attributes states
            GLAttribute& pos = *(prog.m_attributes["aPos"]);
            ASSERT_STREQ("aPos", pos.cname());
            ASSERT_EQ(-1, pos.m_handle);
            ASSERT_EQ(GL_FLOAT, pos.m_target);
            ASSERT_EQ(3, pos.m_size);
            ASSERT_EQ(prog.m_handle, pos.m_program);
            ASSERT_EQ(0, pos.m_index);
            ASSERT_EQ(0, pos.m_stride);
            ASSERT_EQ(0, pos.m_offset);
            ASSERT_EQ(true, pos.m_need_setup);
            ASSERT_EQ(true, pos.m_need_create);
            ASSERT_EQ(false, pos.m_need_update);

            GLUniform<Vector4f>& color = prog.uniform<Vector4f>("ourColor");
            ASSERT_STREQ("ourColor", color.cname());
            ASSERT_EQ(0, color.m_handle);
            ASSERT_EQ(GL_FLOAT_VEC4, color.m_target);
            ASSERT_EQ(4, color.m_size);
            ASSERT_EQ(prog.m_handle, color.m_program);
            // ASSERT_FLOAT_EQ(0.0f, color.m_data.x);
            // ASSERT_FLOAT_EQ(0.0f, color.m_data.y);
            // ASSERT_FLOAT_EQ(0.0f, color.m_data.z);
            // ASSERT_FLOAT_EQ(0.0f, color.m_data.w);
            ASSERT_EQ(false, color.m_need_setup);
            ASSERT_EQ(false, color.m_need_create);
            ASSERT_EQ(false, color.m_need_update);
        });
}

// Test we cannot create uniform before the prog is compiled
TEST(TestGLPrograms, testCreateUniformBeforeCompilation)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader vs;
            GLFragmentShader fs;
            GLProgram prog("prog");

            // Add uniform before compiling
            prog.uniform<Vector4f>("ourColor") =
                Vector4f(1.0f, 2.0f, 3.0f, 4.0f);
            GLUniform<Vector4f>& color1 = prog.uniform<Vector4f>("ourColor");
            ASSERT_STREQ("ourColor", color1.cname());
            ASSERT_EQ(-1, color1.m_handle);
            ASSERT_EQ(GL_FLOAT_VEC4, color1.m_target);
            ASSERT_EQ(4, color1.m_size);
            ASSERT_EQ(prog.m_handle, color1.m_program);
            ASSERT_FLOAT_EQ(1.0f, color1.m_data.x);
            ASSERT_FLOAT_EQ(2.0f, color1.m_data.y);
            ASSERT_FLOAT_EQ(3.0f, color1.m_data.z);
            ASSERT_FLOAT_EQ(4.0f, color1.m_data.w);
            ASSERT_EQ(true, color1.m_need_setup);
            ASSERT_EQ(true, color1.m_need_create);
            ASSERT_EQ(true, color1.m_need_update);

            // Pathes for finding shaders
            vs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");
            fs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");

            // Load GLSL codes
            ASSERT_EQ(true, vs.read("test5.vs"));
            ASSERT_EQ(true, fs.read("test5.fs"));

            ASSERT_EQ(true, prog.compile(vs, fs));
            ASSERT_EQ(true, prog.compiled());

            // Check states for the uniform
            GLUniform<Vector4f>& color2 = prog.uniform<Vector4f>("ourColor");
            ASSERT_STREQ("ourColor", color2.cname());
            ASSERT_EQ(0, color2.m_handle);
            ASSERT_EQ(GL_FLOAT_VEC4, color2.m_target);
            ASSERT_EQ(4, color2.m_size);
            ASSERT_EQ(prog.m_handle, color2.m_program);
            ASSERT_FLOAT_EQ(1.0f, color2.m_data.x);
            ASSERT_FLOAT_EQ(2.0f, color2.m_data.y);
            ASSERT_FLOAT_EQ(3.0f, color2.m_data.z);
            ASSERT_FLOAT_EQ(4.0f, color2.m_data.w);
            ASSERT_EQ(false, color2.m_need_setup);
            ASSERT_EQ(false, color2.m_need_create);
            ASSERT_EQ(false, color2.m_need_update);
        });
}

// Test we cannot create uniform with wrong typr before the prog is compiled
TEST(TestGLPrograms, testCreateUniformBeforeCompilationWrongType)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader vs;
            GLFragmentShader fs;
            GLProgram prog("prog");

            // Add uniform with wrong type before compiling
            prog.uniform<float>("ourColor") = 42.0f;
            GLUniform<float>& color1 = prog.uniform<float>("ourColor");
            ASSERT_STREQ("ourColor", color1.cname());
            ASSERT_EQ(-1, color1.m_handle);
            ASSERT_EQ(GL_FLOAT, color1.m_target);
            ASSERT_EQ(1, color1.m_size);
            ASSERT_EQ(prog.m_handle, color1.m_program);
            ASSERT_FLOAT_EQ(42.0f, color1.m_data);
            ASSERT_EQ(true, color1.m_need_setup);
            ASSERT_EQ(true, color1.m_need_create);
            ASSERT_EQ(true, color1.m_need_update);

            // Pathes for finding shaders
            vs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");
            fs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");

            // Load GLSL codes. ourColor is vec4 not float!
            ASSERT_EQ(true, vs.read("test5.vs"));
            ASSERT_EQ(true, fs.read("test5.fs"));
            ASSERT_EQ(false, prog.compile(vs, fs));
            ASSERT_EQ(false, prog.compiled());
            EXPECT_THAT(prog.strerror().c_str(), HasSubstr("mismatch type"));
            std::vector<std::string> list;
            ASSERT_EQ(0_z, prog.getFailedShaders(list, true));
            ASSERT_EQ(0_z,
                      list.size()); // Shaders are not faulty but the developper
            ASSERT_EQ(0_z, prog.m_shaders.size());
        });
}

// Test we cannot create uniform when the prog is already compiled
TEST(TestGLPrograms, testCreateUniformAfterCompilation)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader vs;
            GLFragmentShader fs;
            GLProgram prog("prog");

            // Pathes for finding shaders
            vs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");
            fs.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                        "OpenGL/shaders:OpenGL/shaders/include");

            // Load GLSL codes
            ASSERT_EQ(true, vs.read("test5.vs"));
            ASSERT_EQ(true, fs.read("test5.fs"));
            ASSERT_EQ(true, prog.compile(vs, fs));
            ASSERT_EQ(true, prog.compiled());

            try
            {
                prog.uniform<float>("ourColor") = 42.0f;
                ASSERT_TRUE("Exception should have occured");
            }
            catch (...)
            {
            }

            try
            {
                prog.uniform<float>("foobar") = 42.0f;
                ASSERT_TRUE("Exception should have occured");
            }
            catch (...)
            {
            }
        });
}

// TODO tester release()

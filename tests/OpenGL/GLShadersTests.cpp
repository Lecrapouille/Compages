//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "OpenGL/Context/OpenGL.hpp"
#include "main.hpp"
#define protected public
#define private public
#include "OpenGL/Shaders/Shaders.hpp"
#undef protected
#undef private

using namespace glwrap;

// TODO tester release()

TEST(TestGLShaders, TestCreatorsNoName)
{
    // No OpenGL context
    {
        GLVertexShader vs;
        GLFragmentShader fs;
        GLGeometryShader gs;

        // Check GLObject states
        ASSERT_STREQ("embedded_vertex_shader", vs.cname());
        ASSERT_STREQ("embedded_fragment_shader", fs.cname());
        ASSERT_STREQ("embedded_geometry_shader", gs.cname());
        ASSERT_EQ(0, vs.m_handle);
        ASSERT_EQ(GL_VERTEX_SHADER, vs.m_target);
        ASSERT_EQ(GL_FRAGMENT_SHADER, fs.m_target);
        ASSERT_EQ(GL_GEOMETRY_SHADER, gs.m_target);
        ASSERT_EQ(true, vs.m_need_setup);
        ASSERT_EQ(true, vs.m_need_create);
        ASSERT_EQ(false, vs.m_need_update);

        // Check GLShader states
        ASSERT_EQ(true, vs.m_code.empty());
        ASSERT_STREQ("", vs.m_code.c_str());
        ASSERT_EQ(false, vs.loaded());
        ASSERT_STREQ("embedded_vertex_shader", vs.m_file_name.c_str());
        ASSERT_STREQ("embedded_fragment_shader", fs.m_file_name.c_str());
        ASSERT_STREQ("embedded_geometry_shader", gs.m_file_name.c_str());
        ASSERT_EQ(true, vs.m_error.empty());
        ASSERT_STREQ("", vs.strerror().c_str());
    }

    OpenGLContext context(
        []()
        {
            GLVertexShader vs;
            GLFragmentShader fs;
            GLGeometryShader gs;

            // Check GLObject states
            ASSERT_STREQ("embedded_vertex_shader", vs.cname());
            ASSERT_STREQ("embedded_fragment_shader", fs.cname());
            ASSERT_STREQ("embedded_geometry_shader", gs.cname());
            ASSERT_EQ(0, vs.m_handle);
            ASSERT_EQ(GL_VERTEX_SHADER, vs.m_target);
            ASSERT_EQ(GL_FRAGMENT_SHADER, fs.m_target);
            ASSERT_EQ(GL_GEOMETRY_SHADER, gs.m_target);
            ASSERT_EQ(true, vs.m_need_setup);
            ASSERT_EQ(true, vs.m_need_create);
            ASSERT_EQ(false, vs.m_need_update);

            // Check GLShader states
            ASSERT_EQ(true, vs.m_code.empty());
            ASSERT_STREQ("", vs.m_code.c_str());
            ASSERT_EQ(false, vs.loaded());
            ASSERT_STREQ("embedded_vertex_shader", vs.m_file_name.c_str());
            ASSERT_STREQ("embedded_fragment_shader", fs.m_file_name.c_str());
            ASSERT_STREQ("embedded_geometry_shader", gs.m_file_name.c_str());
            ASSERT_EQ(true, vs.m_error.empty());
            ASSERT_STREQ("", vs.strerror().c_str());
        });
}

TEST(TestGLShaders, TestCreatorsWithName)
{
    GLVertexShader vs("vs");
    GLFragmentShader fs("fs");
    GLGeometryShader gs("gs");

    ASSERT_STREQ("vs", vs.cname());
    ASSERT_STREQ("fs", fs.cname());
    ASSERT_STREQ("gs", gs.cname());
    ASSERT_STREQ("vs", vs.m_file_name.c_str());
    ASSERT_STREQ("fs", fs.m_file_name.c_str());
    ASSERT_STREQ("gs", gs.m_file_name.c_str());
}

TEST(TestGLShaders, TestConstructCode)
{
    GLVertexShader shader;

    shader = "foo\nbar";
    ASSERT_STREQ("foo\nbar", shader.code().c_str());
    ASSERT_EQ(true, shader.loaded());
    try
    {
        shader.throw_if_not_loaded();
    }
    catch (...)
    {
        ASSERT_TRUE("Exception should have not occured");
    }
    try
    {
        shader.throw_if_already_compiled();
    }
    catch (...)
    {
        ASSERT_TRUE("Exception should have not occured");
    }

    shader << std::endl << "line 1" << std::endl;
    ASSERT_STREQ("foo\nbar\nline 1\n", shader.code().c_str());

    shader += "line 2";
    shader += "\n"; // FIXME std::endl;
    ASSERT_STREQ("foo\nbar\nline 1\nline 2\n", shader.code().c_str());

    "header\n" >> shader;
    ASSERT_STREQ("header\nfoo\nbar\nline 1\nline 2\n", shader.code().c_str());

    shader = "";
    ASSERT_STREQ("", shader.code().c_str());
    ASSERT_EQ(false, shader.loaded());

    shader = "1";
    ASSERT_STREQ("1", shader.code().c_str());
    ASSERT_EQ(true, shader.loaded());

    shader.clear();
    ASSERT_STREQ("", shader.code().c_str());
    ASSERT_EQ(false, shader.loaded());

    try
    {
        shader.throw_if_not_loaded();
        ASSERT_TRUE("Exception should have occured");
    }
    catch (...)
    {
    }

    try
    {
        shader.throw_if_already_compiled();
        ASSERT_TRUE("Exception should have occured");
    }
    catch (...)
    {
    }
}

TEST(TestGLShaders, TestReadFile)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader shader;

            ASSERT_EQ(false, shader.read(""));
            ASSERT_EQ(false, shader.loaded());
            ASSERT_STREQ("", shader.code().c_str());
            EXPECT_THAT(shader.strerror().c_str(), HasSubstr("Failed opening"));
            ASSERT_STREQ("", shader.strerror().c_str());

            shader.path.add("tests/OpenGL/shaders:OpenGL/shaders");
            ASSERT_EQ(true, shader.read("test1.txt"));
            ASSERT_EQ(true, shader.loaded());
            ASSERT_STREQ("#include include/file2.txt\n\nhello\n",
                         shader.code().c_str());
            ASSERT_STREQ("", shader.strerror().c_str());

            ASSERT_EQ(true, shader.read("test2.txt"));
            ASSERT_EQ(true, shader.loaded());
            ASSERT_STREQ("#version 330 core\n"
                         "layout (location = 0) in vec3 position;\n\n"
                         "// Include other files\n"
                         "#include include/functions.incl\n"
                         "#include include/uniforms.incl\n\n"
                         "void main()\n"
                         "{\n"
                         "    position += doFancyCalculationA() * offsetA;\n"
                         "    position += doFancyCalculationB() * offsetB;\n"
                         "    position += doFancyCalculationC() * offsetC;\n"
                         "\n"
                         "    gl_Position = vec4(position, 1.0);\n"
                         "}\n",
                         shader.code().c_str());
            ASSERT_STREQ("", shader.strerror().c_str());

            ASSERT_EQ(false, shader.read("this_file_does_not_exist.txt"));
            ASSERT_EQ(false, shader.loaded());
            ASSERT_STREQ("", shader.code().c_str());
            EXPECT_THAT(shader.strerror().c_str(), HasSubstr("Failed opening"));
            ASSERT_STREQ("", shader.strerror().c_str());
        });
}

TEST(TestGLShaders, solveIncludes1)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader shader;

            shader.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                            "OpenGL/shaders:OpenGL/shaders/include");
            ASSERT_EQ(true, shader.read("test1.txt"));
            // Ignore already included files
            ASSERT_EQ(true, shader.solveIncludes());
            ASSERT_STREQ("\nhello\n", shader.code().c_str());
            ASSERT_STREQ("", shader.strerror().c_str());
        });
}

TEST(TestGLShaders, solveIncludes2)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader shader;

            shader.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                            "OpenGL/shaders:OpenGL/shaders/include");
            ASSERT_EQ(true, shader.read("test3.txt"));
            // Ignore already included files
            ASSERT_EQ(true, shader.solveIncludes());
            ASSERT_STREQ("\n fooo bar\nhello\n", shader.code().c_str());
            ASSERT_STREQ("", shader.strerror().c_str());
        });
}

TEST(TestGLShaders, solveIncludes3)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader shader;

            shader.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                            "OpenGL/shaders:OpenGL/shaders/include");
            ASSERT_EQ(true, shader.read("test2.txt"));
            ASSERT_EQ(true, shader.solveIncludes());
            ASSERT_STREQ("#version 330 core\n"
                         "layout (location = 0) in vec3 position;\n\n"
                         "// Include other files\n"
                         "vec3 doFancyCalculationA()\n"
                         "{\n"
                         "    return vec3(1.0, 0.0, 1.0);\n"
                         "}\n\n"
                         "vec3 doFancyCalculationB()\n"
                         "{\n"
                         "    return vec3(0.0, 1.0, 0.0);\n"
                         "}\n\n"
                         "vec3 doFancyCalculationC()\n"
                         "{\n"
                         "    return vec3(0.0, 0.0, 1.0);\n"
                         "}\n"
                         "uniform vec3 offsetA;\n"
                         "uniform vec3 offsetB;\n"
                         "uniform vec3 offsetC;\n\n"
                         "void main()\n"
                         "{\n"
                         "    position += doFancyCalculationA() * offsetA;\n"
                         "    position += doFancyCalculationB() * offsetB;\n"
                         "    position += doFancyCalculationC() * offsetC;\n\n"
                         "    gl_Position = vec4(position, 1.0);\n"
                         "}\n",
                         shader.code().c_str());
            ASSERT_STREQ("", shader.strerror().c_str());
        });
}

TEST(TestGLShaders, testCompilation)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader shader;

            shader.path.add("tests/OpenGL/shaders:tests/OpenGL/shaders/include:"
                            "OpenGL/shaders:OpenGL/shaders/include");
            ASSERT_EQ(true, shader.read("test4.vs"));
            ASSERT_EQ(true, shader.compile());
            ASSERT_EQ(true, shader.compiled());
            ASSERT_EQ(true, shader.loaded());
            ASSERT_STREQ("#version 330 core\n\n"
                         "in vec2 position;\n"
                         "in vec3 color;\n\n"
                         "out struct v2f_s { vec3 color; } v2f;\n\n"
                         "void main() {\n"
                         "  v2f.color = color;\n"
                         "  gl_Position = vec4(position, 0.0, 1.0);\n"
                         "}\n",
                         shader.code().c_str());
            ASSERT_STREQ("", shader.strerror().c_str());
            ASSERT_EQ(1, shader.m_handle);
            ASSERT_EQ(GL_VERTEX_SHADER, shader.m_target);
            ASSERT_EQ(false, shader.m_need_setup);
            ASSERT_EQ(false, shader.m_need_create);
            ASSERT_EQ(false, shader.m_need_update);

            // Again => compilation ignored
            ASSERT_EQ(true, shader.compile());
        });
}

TEST(TestGLShaders, testFailedCompilation1)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader shader;

            shader = "#version 330 core\n"
                     "in vec2 position;\n"
                     "in vec3 color;\n"
                     "out struct v2f_s { vec3 color; } v2f\n" // missing ';'
                     "void main() {\n"
                     "  v2f.color = color;\n"
                     "  gl_Position = vec4(position, 0.0, 1.0);\n"
                     "}\n";
            ASSERT_EQ(false, shader.compile());
            ASSERT_EQ(false, shader.compiled());
            ASSERT_EQ(true, shader.loaded());
            ASSERT_STREQ("#version 330 core\n"
                         "in vec2 position;\n"
                         "in vec3 color;\n"
                         "out struct v2f_s { vec3 color; } v2f\n"
                         "void main() {\n"
                         "  v2f.color = color;\n"
                         "  gl_Position = vec4(position, 0.0, 1.0);\n"
                         "}\n",
                         shader.code().c_str());
            EXPECT_THAT(shader.strerror().c_str(),
                        HasSubstr("Failed compiling"));
            ASSERT_EQ(1, shader.m_handle);
            ASSERT_EQ(GL_VERTEX_SHADER, shader.m_target);
            ASSERT_EQ(true, shader.m_need_setup);
            ASSERT_EQ(false, shader.m_need_create);
            ASSERT_EQ(false, shader.m_need_update);
        });
}

TEST(TestGLShaders, testFailedCompilation2)
{
    OpenGLContext context(
        []()
        {
            GLVertexShader shader;

            ASSERT_EQ(false, shader.compile());
            ASSERT_EQ(false, shader.compiled());
            ASSERT_EQ(false, shader.loaded());
            EXPECT_THAT(shader.strerror().c_str(),
                        HasSubstr("Failed compiling"));
            ASSERT_EQ(1, shader.m_handle);
            ASSERT_EQ(GL_VERTEX_SHADER, shader.m_target);
            ASSERT_EQ(true, shader.m_need_setup);
            ASSERT_EQ(false, shader.m_need_create);
            ASSERT_EQ(false, shader.m_need_update);
        });
}

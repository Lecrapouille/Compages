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

#include "00_Window/00_BasicWindow.hpp"
#include "00_Window/01_BasicWindowIOEvents.hpp"
#include "00_Window/02_BasicWindowImGui.hpp"

#include "01_Core/00_ColorfulTriangle.hpp"
#include "01_Core/01_DynamicTriangle.hpp"
#include "01_Core/02_TexturedTriangle.hpp"
#include "01_Core/03_MultiTexturedSquare.hpp"
#include "01_Core/04_IndexedQuad.hpp"
#include "01_Core/05_RotatingQuad.hpp"
#include "01_Core/06_IndexedSphere.hpp"
#include "01_Core/07_MultipleObjects.hpp"
#include "01_Core/08_TerrainTexture3D.hpp"
#include "01_Core/09_SkyBoxTextureCube.hpp"
#include "01_Core/10_SkyBoxShape.hpp"
#include "01_Core/11_ComplexShader.hpp"
#include "01_Core/12_BasicLighting.hpp"
#include "01_Core/13_PostProdFrameBuffer.hpp"

#include "02_SceneGraph/00_SG_Base.hpp"
#include "02_SceneGraph/02_SG_MaterialsAndShapes.hpp"
#include "02_SceneGraph/04_SG_AnimatedModel.hpp"

#include "03_ThreeJs/00_MiscLookAt.hpp"

#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <thread>

#define MAX_EXAMPLES 27
#define WIDTH 800
#define HEIGHT 600

//------------------------------------------------------------------------------
enum class Examples
{
    BasicWindow,
    BasicWindowIOEvents,
    BasicWindowImGui,
    ColorfulTriangle,
    DynamicTriangle,
    TexturedTriangle,
    MultiTexturedSquare,
    IndexedQuad,
    RotatingQuad,
    IndexedSphere,
    MultipleObjects,
    TerrainTexture3D,
    SkyBoxTextureCube,
    SkyBoxShape,
    ComplexShader,
    BasicLighting,
    PostProdFrameBuffer,
    SGBase,
    SGMatAndShape,
    SGAnimatedModel,
    MiscLookAt,
};

//------------------------------------------------------------------------------
__attribute__((__noreturn__)) static void usage(char* argv[])
{
    std::cout << "Usage: " << std::endl
              << "  " << argv[0] << " <integer>" << std::endl;
    std::cout << "Where: <integer> is the example id (0 .. " << MAX_EXAMPLES
              << "): " << std::endl;
    std::cout << "Window API:" << std::endl;
    std::cout << "  " << int(Examples::BasicWindow) << ": "
              << BasicWindow::info() << std::endl;
    std::cout << "  " << int(Examples::BasicWindowIOEvents) << ": "
              << BasicWindowIOEvents::info() << std::endl;
    std::cout << "  " << int(Examples::BasicWindowImGui) << ": "
              << BasicWindowImGui::info() << std::endl;
    std::cout << "OpenGL wrapper API:" << std::endl;
    std::cout << "  " << int(Examples::ColorfulTriangle) << ": "
              << ColorfulTriangle::info() << std::endl;
    std::cout << "  " << int(Examples::DynamicTriangle) << ": "
              << DynamicTriangle::info() << std::endl;
    std::cout << "  " << int(Examples::TexturedTriangle) << ": "
              << TexturedTriangle::info() << std::endl;
    std::cout << "  " << int(Examples::MultiTexturedSquare) << ": "
              << MultiTexturedSquare::info() << std::endl;
    std::cout << "  " << int(Examples::IndexedQuad) << ": "
              << IndexedQuad::info() << std::endl;
    std::cout << "  " << int(Examples::RotatingQuad) << ": "
              << RotatingQuad::info() << std::endl;
    std::cout << "  " << int(Examples::IndexedSphere) << ": "
              << IndexedSphere::info() << std::endl;
    std::cout << "  " << int(Examples::MultipleObjects) << ": "
              << MultipleObjects::info() << std::endl;
    std::cout << "  " << int(Examples::TerrainTexture3D) << ": "
              << TerrainTexture3D::info() << std::endl;
    std::cout << "  " << int(Examples::SkyBoxTextureCube) << ": "
              << SkyBoxTextureCube::info() << std::endl;
    std::cout << "  " << int(Examples::SkyBoxShape) << ": "
              << SkyBoxShape::info() << std::endl;
    std::cout << "  " << int(Examples::ComplexShader) << ": "
              << ComplexShader::info() << std::endl;
    std::cout << "  " << int(Examples::BasicLighting) << ": "
              << BasicLighting::info() << std::endl;
    std::cout << "  " << int(Examples::PostProdFrameBuffer) << ": "
              << PostProdFrameBuffer::info() << std::endl;
    std::cout << "SceneGraph API:" << std::endl;
    std::cout << "  " << int(Examples::SGBase) << ": " << SGBase::info()
              << std::endl;
    std::cout << "  " << int(Examples::SGMatAndShape) << ": "
              << SGMatAndShape::info() << std::endl;
    std::cout << "  " << int(Examples::SGAnimatedModel) << ": "
              << SGAnimatedModel::info() << std::endl;
    std::cout << "Examples from Three.js:" << std::endl;
    std::cout << "  " << int(Examples::MiscLookAt) << ": " << MiscLookAt::info()
              << std::endl;
    exit(EXIT_FAILURE);
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Need the id of the desired example
    if (argc <= 1)
    {
        std::cerr << "Missing example id !" << std::endl;
        usage(argv);
    }

    // Get the id of the example
    char* endptr;
    long int id = strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr != '\0' ||
        ((id == LONG_MIN || id == LONG_MAX) && errno == ERANGE))
    {
        std::cerr << "'" << argv[1] << "' is not a valid example id !"
                  << std::endl;
        usage(argv);
    }

    std::unique_ptr<GLWindow> app = nullptr;
    try
    {
        switch (Examples(id))
        {
            case Examples::BasicWindow:
                app =
                    std::make_unique<BasicWindow>(WIDTH, HEIGHT, "BasicWindow");
                break;
            case Examples::BasicWindowIOEvents:
                app = std::make_unique<BasicWindowIOEvents>(
                    WIDTH, HEIGHT, "IO events");
                break;
            case Examples::BasicWindowImGui:
                app = std::make_unique<BasicWindowImGui>(
                    WIDTH, HEIGHT, "DearImGui");
                break;
            case Examples::ColorfulTriangle:
                app = std::make_unique<ColorfulTriangle>(
                    WIDTH, HEIGHT, "Colorful Triangle");
                break;
            case Examples::DynamicTriangle:
                app = std::make_unique<DynamicTriangle>(
                    WIDTH, HEIGHT, "Dynamic Triangle");
                break;
            case Examples::TexturedTriangle:
                app = std::make_unique<TexturedTriangle>(
                    WIDTH, HEIGHT, "Textured Triangle");
                break;
            case Examples::MultiTexturedSquare:
                app = std::make_unique<MultiTexturedSquare>(
                    WIDTH, HEIGHT, "Multi Textured Triangle");
                break;
            case Examples::IndexedQuad:
                app = std::make_unique<IndexedQuad>(WIDTH, HEIGHT, "EBO Quad");
                break;
            case Examples::RotatingQuad:
                app = std::make_unique<RotatingQuad>(
                    WIDTH, HEIGHT, "Rotating Quad");
                break;
            case Examples::IndexedSphere:
                app = std::make_unique<IndexedSphere>(
                    WIDTH, HEIGHT, "EBO Sphere");
                break;
            case Examples::MultipleObjects:
                app = std::make_unique<MultipleObjects>(
                    WIDTH, HEIGHT, "Multiple Objects");
                break;
            case Examples::TerrainTexture3D:
                app = std::make_unique<TerrainTexture3D>(
                    WIDTH, HEIGHT, "Terrain 3D Texture");
                break;
            case Examples::SkyBoxTextureCube:
                app = std::make_unique<SkyBoxTextureCube>(
                    WIDTH, HEIGHT, "SkyBox Cube Texture");
                break;
            case Examples::SkyBoxShape:
                app = std::make_unique<SkyBoxShape>(
                    WIDTH, HEIGHT, "Shape inside a skybox");
                break;
            case Examples::ComplexShader:
                app = std::make_unique<ComplexShader>(
                    WIDTH, HEIGHT, "Complex Shader");
                break;
            case Examples::BasicLighting:
                app = std::make_unique<BasicLighting>(
                    WIDTH, HEIGHT, "Basic Lighting");
                break;
            case Examples::PostProdFrameBuffer:
                app = std::make_unique<PostProdFrameBuffer>(
                    WIDTH, HEIGHT, "PostProd FrameBuffer");
                break;
            case Examples::SGBase:
                app = std::make_unique<SGBase>(
                    WIDTH, HEIGHT, "Base of Scene Graph");
                break;
            case Examples::SGMatAndShape:
                app = std::make_unique<SGMatAndShape>(
                    WIDTH, HEIGHT, "Scene Graph of Shapes and Meterials");
                break;
            case Examples::SGAnimatedModel:
                app = std::make_unique<SGAnimatedModel>(
                    WIDTH, HEIGHT, "Animated Model");
                break;
            case Examples::MiscLookAt:
                app = std::make_unique<MiscLookAt>(
                    WIDTH, HEIGHT, "Three.js example");
                break;
            default:
                std::cerr << "Incorrect example id !" << std::endl;
                std::cerr << "'" << argv[1] << "' is not a valid example id !"
                          << std::endl;
                usage(argv);
                return EXIT_FAILURE;
        }

        return app->run() ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    catch (const GL::Exception& e)
    {
        std::cerr << "Caught exception from constructors: " << e.message()
                  << std::endl;
    }

    return EXIT_FAILURE;
}

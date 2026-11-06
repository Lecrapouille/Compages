// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Gallery.hpp"

#include "01_FirstSteps/01a_Dummy.hpp"
#include "01_FirstSteps/01b_ClearScreen.hpp"
#include "01_FirstSteps/01c_Triangle.hpp"
#include "01_FirstSteps/01d_InterleavedTriangle.hpp"
#include "01_FirstSteps/01e_BorrowedTriangle.hpp"
#include "01_FirstSteps/01f_VertexSyntax.hpp"
#include "02_ChangingData/02a_CpuGpuSync.hpp"
#include "02_ChangingData/02b_DynamicGeometry.hpp"
#include "02_ChangingData/02c_HeightMap.hpp"
#include "02_ChangingData/02d_Lorenz.hpp"
#include "03_Images/03a_TexturedQuad.hpp"
#include "03_Images/03b_MultiTextureBlend.hpp"
#include "03_Images/03c_DepthAndTransforms.hpp"
#include "03_Images/03d_PointClouds.hpp"
#include "03_Images/03e_Terrain3D.hpp"
#include "04_Passes/04a_Mandelbrot.hpp"
#include "04_Passes/04b_ComplexShader.hpp"
#include "04_Passes/04c_MultiPassMesh.hpp"
#include "04_Passes/04d_RenderToTexture.hpp"
#include "04_Passes/04e_PostProcess.hpp"
#include "05_Simulation/05a_Compute.hpp"
#include "05_Simulation/05b_GameOfLife.hpp"
#include "05_Simulation/05c_GrayScott.hpp"
#include "05_Simulation/05d_ComputeParticles.hpp"
#include "05_Simulation/05e_SpiralGalaxy.hpp"
#include "05_Simulation/05f_Galaxy.hpp"
#include "05_Simulation/05g_NeuralNetwork.hpp"
#include "06_Scene/06a_FirstScene.hpp"
#include "06_Scene/06b_Transforms.hpp"
#include "06_Scene/06c_Hierarchy.hpp"
#include "06_Scene/06d_Lights.hpp"
#include "06_Scene/06e_TexturedSpheres.hpp"
#include "06_Scene/06f_GeometryShowcase.hpp"
#include "06_Scene/06g_TextureGallery.hpp"
#include "06_Scene/06h_Skybox.hpp"
#include "06_Scene/06i_CameraPick.hpp"
#include "06_Scene/06j_SplitViews.hpp"
#include "06_Scene/06k_MiscLookAt.hpp"
#include "06_Scene/06l_CanvasPaint.hpp"
#include "07_World/07a_HeadlessWorld.hpp"
#include "07_World/07b_Routines.hpp"
#include "07_World/07c_MovingRobot.hpp"
#include "07_World/07d_AnimatedModel.hpp"
#include "07_World/07e_ExternalPhysics.hpp"
#include "08_Assets/08a_GltfModel.hpp"
#include "08_Assets/08b_GltfAnimation.hpp"
#include "08_Assets/08c_PrefabAndSave.hpp"
#include "08_Assets/08d_RobotArm.hpp"
#include "09_Performance/09a_SpriteBatch.hpp"
#include "09_Performance/09b_ManyCubes.hpp"
#include "09_Performance/09c_IndirectDraw.hpp"
#include "10_Complete/10a_MvpDemo.hpp"
#include "10_Complete/10b_DoomLike.hpp"
#include "10_Complete/10c_SweatAndDecals.hpp"

// One record per active source, in gallery order:
// type, runtime name, chapter folder, source, former demos covered,
// budget, functions included in that budget.
#define COMPAGES_EXAMPLE_MANIFEST(X)                                           \
    X(Dummy, "01a_Dummy", "01_FirstSteps", "01_FirstSteps/01a_Dummy.cpp", "-", 0, "-") \
    X(ClearScreen, "01b_ClearScreen", "01_FirstSteps", "01_FirstSteps/01b_ClearScreen.cpp", "01_ClearScreen", 0, "-") \
    X(Triangle, "01c_Triangle", "01_FirstSteps", "01_FirstSteps/01c_Triangle.cpp", "02_Triangle", 30, "Triangle::setUp|Triangle::draw") \
    X(InterleavedTriangle, "01d_InterleavedTriangle", "01_FirstSteps", "01_FirstSteps/01d_InterleavedTriangle.cpp", "-", 30, "InterleavedTriangle::setUp|InterleavedTriangle::draw") \
    X(BorrowedTriangle, "01e_BorrowedTriangle", "01_FirstSteps", "01_FirstSteps/01e_BorrowedTriangle.cpp", "-", 0, "-") \
    X(VertexSyntax, "01f_VertexSyntax", "01_FirstSteps", "01_FirstSteps/01f_VertexSyntax.cpp", "-", 0, "-") \
    X(CpuGpuSync, "02a_CpuGpuSync", "02_ChangingData", "02_ChangingData/02a_CpuGpuSync.cpp", "-", 0, "-") \
    X(DynamicTriangle, "02b_DynamicGeometry", "02_ChangingData", "02_ChangingData/02b_DynamicGeometry.cpp", "03_DynamicTriangle", 0, "-") \
    X(HeightMap, "02c_HeightMap", "02_ChangingData", "02_ChangingData/02c_HeightMap.cpp", "09_HeightMap", 0, "-") \
    X(Lorenz, "02d_Lorenz", "02_ChangingData", "02_ChangingData/02d_Lorenz.cpp", "12_Lorenz", 0, "-") \
    X(TexturedQuad, "03a_TexturedQuad", "03_Images", "03_Images/03a_TexturedQuad.cpp", "04_TexturedQuad", 40, "TexturedQuad::makeTexture|TexturedQuad::setUp|TexturedQuad::draw") \
    X(MultiTextureBlend, "03b_MultiTextureBlend", "03_Images", "03_Images/03b_MultiTextureBlend.cpp", "28_MultiTextureBlend", 0, "-") \
    X(IndexedCube, "03c_DepthAndTransforms", "03_Images", "03_Images/03c_DepthAndTransforms.cpp", "05_IndexedCube", 0, "-") \
    X(PointSphere, "03d_PointClouds", "03_Images", "03_Images/03d_PointClouds.cpp", "29_PointSphere", 0, "-") \
    X(Terrain3D, "03e_Terrain3D", "03_Images", "03_Images/03e_Terrain3D.cpp", "30_Terrain3D", 0, "-") \
    X(Mandelbrot, "04a_Mandelbrot", "04_Passes", "04_Passes/04a_Mandelbrot.cpp", "08_Mandelbrot", 0, "-") \
    X(ComplexShader, "04b_ComplexShader", "04_Passes", "04_Passes/04b_ComplexShader.cpp", "31_ComplexShader", 0, "-") \
    X(MultiPassMesh, "04c_MultiPassMesh", "04_Passes", "04_Passes/04c_MultiPassMesh.cpp", "06_MultiPassMesh", 0, "-") \
    X(RenderToTexture, "04d_RenderToTexture", "04_Passes", "04_Passes/04d_RenderToTexture.cpp", "07_RenderToTexture", 0, "-") \
    X(PostProcess, "04e_PostProcess", "04_Passes", "04_Passes/04e_PostProcess.cpp", "32_PostProcess", 0, "-") \
    X(IntroCompute, "05a_Compute", "05_Simulation", "05_Simulation/05a_Compute.cpp", "-", 0, "-") \
    X(GameOfLife, "05b_GameOfLife", "05_Simulation", "05_Simulation/05b_GameOfLife.cpp", "10_GameOfLife", 0, "-") \
    X(GrayScott, "05c_GrayScott", "05_Simulation", "05_Simulation/05c_GrayScott.cpp", "11_GrayScott", 0, "-") \
    X(ComputeParticles, "05d_ComputeParticles", "05_Simulation", "05_Simulation/05d_ComputeParticles.cpp", "13_ComputeParticles", 0, "-") \
    X(SpiralGalaxy, "05e_SpiralGalaxy", "05_Simulation", "05_Simulation/05e_SpiralGalaxy.cpp", "-", 0, "-") \
    X(Galaxy, "05f_Galaxy", "05_Simulation", "05_Simulation/05f_Galaxy.cpp", "14_Galaxy", 0, "-") \
    X(NeuralNetwork, "05g_NeuralNetwork", "05_Simulation", "05_Simulation/05g_NeuralNetwork.cpp", "-", 0, "-") \
    X(FirstScene, "06a_FirstScene", "06_Scene", "06_Scene/06a_FirstScene.cpp", "-", 15, "FirstScene::setUp|FirstScene::draw") \
    X(Transforms, "06b_Transforms", "06_Scene", "06_Scene/06b_Transforms.cpp", "-", 15, "Transforms::setUp|Transforms::draw") \
    X(Hierarchy, "06c_Hierarchy", "06_Scene", "06_Scene/06c_Hierarchy.cpp", "-", 15, "Hierarchy::setUp|Hierarchy::draw") \
    X(Lights, "06d_Lights", "06_Scene", "06_Scene/06d_Lights.cpp", "-", 15, "Lights::setUp|Lights::draw") \
    X(TexturedSpheres, "06e_TexturedSpheres", "06_Scene", "06_Scene/06e_TexturedSpheres.cpp", "22_TexturedSpheres", 0, "-") \
    X(GeometryShowcase, "06f_GeometryShowcase", "06_Scene", "06_Scene/06f_GeometryShowcase.cpp", "33_GeometryShowcase", 0, "-") \
    X(TextureGallery, "06g_TextureGallery", "06_Scene", "06_Scene/06g_TextureGallery.cpp", "27_TextureGallery", 0, "-") \
    X(Skybox, "06h_Skybox", "06_Scene", "06_Scene/06h_Skybox.cpp", "26_Skybox", 0, "-") \
    X(CameraPick, "06i_CameraPick", "06_Scene", "06_Scene/06i_CameraPick.cpp", "20_CameraPick", 0, "-") \
    X(SplitViews, "06j_SplitViews", "06_Scene", "06_Scene/06j_SplitViews.cpp", "19_SplitViews", 0, "-") \
    X(MiscLookAt, "06k_MiscLookAt", "06_Scene", "06_Scene/06k_MiscLookAt.cpp", "25_MiscLookAt", 0, "-") \
    X(CanvasPaint, "06l_CanvasPaint", "06_Scene", "06_Scene/06l_CanvasPaint.cpp", "-", 0, "-") \
    X(HeadlessWorld, "07a_HeadlessWorld", "07_World", "07_World/07a_HeadlessWorld.cpp", "-", 0, "-") \
    X(Routines, "07b_Routines", "07_World", "07_World/07b_Routines.cpp", "17_MovingRobot", 0, "-") \
    X(MovingRobot, "07c_MovingRobot", "07_World", "07_World/07c_MovingRobot.cpp", "17_MovingRobot", 0, "-") \
    X(AnimatedModel, "07d_AnimatedModel", "07_World", "07_World/07d_AnimatedModel.cpp", "34_AnimatedModel", 0, "-") \
    X(ExternalPhysics, "07e_ExternalPhysics", "07_World", "07_World/07e_ExternalPhysics.cpp", "-", 0, "-") \
    X(GltfModel, "08a_GltfModel", "08_Assets", "08_Assets/08a_GltfModel.cpp", "21_GltfModel", 15, "GltfModel::setUp|GltfModel::draw") \
    X(GltfAnimation, "08b_GltfAnimation", "08_Assets", "08_Assets/08b_GltfAnimation.cpp", "36_GltfAnimation", 0, "-") \
    X(PrefabAndSave, "08c_PrefabAndSave", "08_Assets", "08_Assets/08c_PrefabAndSave.cpp", "23_PrefabAndSave", 0, "-") \
    X(RobotArm, "08d_RobotArm", "08_Assets", "08_Assets/08d_RobotArm.cpp", "-", 0, "-") \
    X(SpriteBatch, "09a_SpriteBatch", "09_Performance", "09_Performance/09a_SpriteBatch.cpp", "15_SpriteBatch", 0, "-") \
    X(ManyCubes, "09b_ManyCubes", "09_Performance", "09_Performance/09b_ManyCubes.cpp", "18_ManyCubes", 0, "-") \
    X(IndirectDraw, "09c_IndirectDraw", "09_Performance", "09_Performance/09c_IndirectDraw.cpp", "16_IndirectDraw", 0, "-") \
    X(MvpDemo, "10a_MvpDemo", "10_Complete", "10_Complete/10a_MvpDemo.cpp", "24_MvpDemo", 20, "MvpDemo::draw") \
    X(DoomLike, "10b_DoomLike", "10_Complete", "10_Complete/10b_DoomLike.cpp", "-", 0, "-") \
    X(SweatAndDecals, "10c_SweatAndDecals", "10_Complete", "10_Complete/10c_SweatAndDecals.cpp", "-", 0, "-")

namespace examples
{

inline void registerManifest(Gallery& p_gallery)
{
#define COMPAGES_REGISTER(TYPE, NAME, TARGET, SOURCE, LEGACY, BUDGET, SCOPE) \
    p_gallery.add<TYPE>(NAME, SOURCE);
    COMPAGES_EXAMPLE_MANIFEST(COMPAGES_REGISTER)
#undef COMPAGES_REGISTER
}

} // namespace examples

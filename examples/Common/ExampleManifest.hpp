// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Gallery.hpp"

#include "00_GettingStarted/00a_Dummy.hpp"
#include "00_GettingStarted/00b_CpuGpuSync.hpp"
#include "00_GettingStarted/00c_Compute.hpp"
#include "00_GettingStarted/01a_ClearScreen.hpp"
#include "00_GettingStarted/01b_Triangle.hpp"
#include "00_GettingStarted/01c_InterleavedTriangle.hpp"
#include "00_GettingStarted/02_DynamicGeometry.hpp"
#include "00_GettingStarted/03a_TexturedQuad.hpp"
#include "00_GettingStarted/03b_MultiTextureBlend.hpp"
#include "00_GettingStarted/04_DepthAndTransforms.hpp"
#include "00_GettingStarted/05a_MultiPassMesh.hpp"
#include "00_GettingStarted/05b_RenderToTexture.hpp"
#include "00_GettingStarted/05c_PostProcess.hpp"
#include "00_GettingStarted/06a_Mandelbrot.hpp"
#include "00_GettingStarted/06b_ComplexShader.hpp"
#include "00_GettingStarted/07_PointClouds.hpp"
#include "10_ScientificAndCompute/10a_HeightMap.hpp"
#include "10_ScientificAndCompute/10b_Terrain3D.hpp"
#include "10_ScientificAndCompute/11a_GameOfLife.hpp"
#include "10_ScientificAndCompute/11b_GrayScott.hpp"
#include "10_ScientificAndCompute/12_ComputeParticles.hpp"
#include "10_ScientificAndCompute/13_Galaxy.hpp"
#include "10_ScientificAndCompute/14_SpiralGalaxy.hpp"
#include "10_ScientificAndCompute/15_Lorenz.hpp"
#include "20_Performance/20a_SpriteBatch.hpp"
#include "20_Performance/20b_ManyCubes.hpp"
#include "20_Performance/21_IndirectDraw.hpp"
#include "30_WorldAndAssets/30_HeadlessWorld.hpp"
#include "30_WorldAndAssets/31_MovingRobot.hpp"
#include "30_WorldAndAssets/32a_SplitViews.hpp"
#include "30_WorldAndAssets/32b_CameraPick.hpp"
#include "30_WorldAndAssets/32c_MiscLookAt.hpp"
#include "30_WorldAndAssets/33a_TexturedSpheres.hpp"
#include "30_WorldAndAssets/33b_TextureGallery.hpp"
#include "30_WorldAndAssets/33c_GeometryShowcase.hpp"
#include "30_WorldAndAssets/34_GltfModel.hpp"
#include "30_WorldAndAssets/35_PrefabAndSave.hpp"
#include "30_WorldAndAssets/36a_AnimatedModel.hpp"
#include "30_WorldAndAssets/36b_GltfAnimation.hpp"
#include "30_WorldAndAssets/37_Skybox.hpp"
#include "30_WorldAndAssets/38_RobotArm.hpp"
#include "50_Complete/50_ThreeJsLike.hpp"
#include "50_Complete/51_Behaviors.hpp"
#include "50_Complete/52_MvpDemo.hpp"
#include "50_Complete/53_DoomLike.hpp"

// One record per active source, in gallery order:
// type, runtime name, conceptual target, source, former demos covered,
// budget, functions included in that budget.
#define COMPAGES_EXAMPLE_MANIFEST(X)                                           \
    X(Dummy, "00a_Dummy", "00_Dummy", "00_GettingStarted/00a_Dummy.cpp", "-", 0, "-") \
    X(CpuGpuSync, "00b_CpuGpuSync", "00_CpuGpuSync", "00_GettingStarted/00b_CpuGpuSync.cpp", "-", 0, "-") \
    X(IntroCompute, "00c_Compute", "00_Compute", "00_GettingStarted/00c_Compute.cpp", "-", 0, "-") \
    X(ClearScreen, "01a_ClearScreen", "01_ClearAndTriangle", "00_GettingStarted/01a_ClearScreen.cpp", "01_ClearScreen", 0, "-") \
    X(Triangle, "01b_Triangle", "01_ClearAndTriangle", "00_GettingStarted/01b_Triangle.cpp", "02_Triangle", 30, "Triangle::setUp|Triangle::draw") \
    X(InterleavedTriangle, "01c_InterleavedTriangle", "01_ClearAndTriangle", "00_GettingStarted/01c_InterleavedTriangle.cpp", "-", 30, "InterleavedTriangle::setUp|InterleavedTriangle::draw") \
    X(DynamicTriangle, "02_DynamicGeometry", "02_DynamicGeometry", "00_GettingStarted/02_DynamicGeometry.cpp", "03_DynamicTriangle", 0, "-") \
    X(TexturedQuad, "03a_TexturedQuad", "03_TexturesAndSampling", "00_GettingStarted/03a_TexturedQuad.cpp", "04_TexturedQuad", 40, "TexturedQuad::makeTexture|TexturedQuad::setUp|TexturedQuad::draw") \
    X(MultiTextureBlend, "03b_MultiTextureBlend", "03_TexturesAndSampling", "00_GettingStarted/03b_MultiTextureBlend.cpp", "28_MultiTextureBlend", 0, "-") \
    X(IndexedCube, "04_DepthAndTransforms", "04_DepthAndTransforms", "00_GettingStarted/04_DepthAndTransforms.cpp", "05_IndexedCube", 0, "-") \
    X(MultiPassMesh, "05a_MultiPassMesh", "05_RenderTargets", "00_GettingStarted/05a_MultiPassMesh.cpp", "06_MultiPassMesh", 0, "-") \
    X(RenderToTexture, "05b_RenderToTexture", "05_RenderTargets", "00_GettingStarted/05b_RenderToTexture.cpp", "07_RenderToTexture", 0, "-") \
    X(PostProcess, "05c_PostProcess", "05_RenderTargets", "00_GettingStarted/05c_PostProcess.cpp", "32_PostProcess", 0, "-") \
    X(Mandelbrot, "06a_Mandelbrot", "06_ProceduralShaders", "00_GettingStarted/06a_Mandelbrot.cpp", "08_Mandelbrot", 0, "-") \
    X(ComplexShader, "06b_ComplexShader", "06_ProceduralShaders", "00_GettingStarted/06b_ComplexShader.cpp", "31_ComplexShader", 0, "-") \
    X(PointSphere, "07_PointClouds", "07_PointClouds", "00_GettingStarted/07_PointClouds.cpp", "29_PointSphere", 0, "-") \
    X(HeightMap, "10a_HeightMap", "10_HeightField", "10_ScientificAndCompute/10a_HeightMap.cpp", "09_HeightMap", 0, "-") \
    X(Terrain3D, "10b_Terrain3D", "10_HeightField", "10_ScientificAndCompute/10b_Terrain3D.cpp", "30_Terrain3D", 0, "-") \
    X(GameOfLife, "11a_GameOfLife", "11_TextureSimulation", "10_ScientificAndCompute/11a_GameOfLife.cpp", "10_GameOfLife", 0, "-") \
    X(GrayScott, "11b_GrayScott", "11_TextureSimulation", "10_ScientificAndCompute/11b_GrayScott.cpp", "11_GrayScott", 0, "-") \
    X(ComputeParticles, "12_ComputeParticles", "12_ComputeParticles", "10_ScientificAndCompute/12_ComputeParticles.cpp", "13_ComputeParticles", 0, "-") \
    X(Galaxy, "13_Galaxy", "13_NBodyGalaxy", "10_ScientificAndCompute/13_Galaxy.cpp", "14_Galaxy", 0, "-") \
    X(SpiralGalaxy, "14_SpiralGalaxy", "14_DensityWave", "10_ScientificAndCompute/14_SpiralGalaxy.cpp", "-", 0, "-") \
    X(Lorenz, "15_Lorenz", "15_StreamedCurves", "10_ScientificAndCompute/15_Lorenz.cpp", "12_Lorenz", 0, "-") \
    X(SpriteBatch, "20a_SpriteBatch", "20_Instancing", "20_Performance/20a_SpriteBatch.cpp", "15_SpriteBatch", 0, "-") \
    X(ManyCubes, "20b_ManyCubes", "20_Instancing", "20_Performance/20b_ManyCubes.cpp", "18_ManyCubes", 0, "-") \
    X(IndirectDraw, "21_IndirectDraw", "21_IndirectCulling", "20_Performance/21_IndirectDraw.cpp", "16_IndirectDraw", 0, "-") \
    X(HeadlessWorld, "30_HeadlessWorld", "30_HeadlessWorld", "30_WorldAndAssets/30_HeadlessWorld.cpp", "-", 0, "-") \
    X(MovingRobot, "31_MovingRobot", "31_HierarchyAndTransforms", "30_WorldAndAssets/31_MovingRobot.cpp", "17_MovingRobot", 0, "-") \
    X(SplitViews, "32a_SplitViews", "32_CamerasAndPicking", "30_WorldAndAssets/32a_SplitViews.cpp", "19_SplitViews", 0, "-") \
    X(CameraPick, "32b_CameraPick", "32_CamerasAndPicking", "30_WorldAndAssets/32b_CameraPick.cpp", "20_CameraPick", 0, "-") \
    X(MiscLookAt, "32c_MiscLookAt", "32_CamerasAndPicking", "30_WorldAndAssets/32c_MiscLookAt.cpp", "25_MiscLookAt", 0, "-") \
    X(TexturedSpheres, "33a_TexturedSpheres", "33_MaterialsAndPrimitives", "30_WorldAndAssets/33a_TexturedSpheres.cpp", "22_TexturedSpheres", 0, "-") \
    X(TextureGallery, "33b_TextureGallery", "33_MaterialsAndPrimitives", "30_WorldAndAssets/33b_TextureGallery.cpp", "27_TextureGallery", 0, "-") \
    X(GeometryShowcase, "33c_GeometryShowcase", "33_MaterialsAndPrimitives", "30_WorldAndAssets/33c_GeometryShowcase.cpp", "33_GeometryShowcase", 0, "-") \
    X(GltfModel, "34_GltfModel", "34_GltfScene", "30_WorldAndAssets/34_GltfModel.cpp", "21_GltfModel", 15, "GltfModel::setUp|GltfModel::draw") \
    X(PrefabAndSave, "35_PrefabAndSave", "35_PrefabsAndSerialization", "30_WorldAndAssets/35_PrefabAndSave.cpp", "23_PrefabAndSave", 0, "-") \
    X(AnimatedModel, "36a_AnimatedModel", "36_AnimationAndSkinning", "30_WorldAndAssets/36a_AnimatedModel.cpp", "34_AnimatedModel", 0, "-") \
    X(GltfAnimation, "36b_GltfAnimation", "36_AnimationAndSkinning", "30_WorldAndAssets/36b_GltfAnimation.cpp", "36_GltfAnimation", 0, "-") \
    X(Skybox, "37_Skybox", "37_EnvironmentAndSkybox", "30_WorldAndAssets/37_Skybox.cpp", "26_Skybox", 0, "-") \
    X(RobotArm, "38_RobotArm", "38_KinematicChains", "30_WorldAndAssets/38_RobotArm.cpp", "-", 0, "-") \
    X(ThreeJsLike, "50_ThreeJsLike", "50_ThreeJsLike", "50_Complete/50_ThreeJsLike.cpp", "-", 15, "ThreeJsLike::setUp|ThreeJsLike::draw") \
    X(Behaviors, "51_Behaviors", "51_Behaviors", "50_Complete/51_Behaviors.cpp", "17_MovingRobot", 0, "-") \
    X(MvpDemo, "52_MvpDemo", "52_Simulation", "50_Complete/52_MvpDemo.cpp", "24_MvpDemo", 20, "MvpDemo::draw") \
    X(DoomLike, "53_DoomLike", "53_DoomLike", "50_Complete/53_DoomLike.cpp", "-", 0, "-")

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

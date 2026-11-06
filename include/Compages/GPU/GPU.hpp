// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The single header to include to use the GPU layer: everything
//! needed to draw and compute on the graphics card.
//!
//! The GPU layer is the lowest of the three layers of Compages. It knows
//! nothing about scenes or objects: it sends shapes, pictures and shaders to
//! the graphics card and asks it to draw or compute. Most programs start from
//! Drawable, and reach for the other headers when they need more control.
//!
//! \code
//! #include "Compages/GPU/GPU.hpp"
//!
//! // The window is yours to create; the library only wants the loader.
//! if (auto ready = compages::gpu::init(
//!         reinterpret_cast<compages::gpu::LoadProc>(glfwGetProcAddress));
//!     !ready)
//! {
//!     std::cerr << ready.error() << std::endl;
//!     return EXIT_FAILURE;
//! }
//! ...
//! compages::gpu::shutdown();
//! \endcode
//!
//! Nothing here exposes the graphics API in use. That is deliberate and
//! enforced: the only file allowed to include glad is the private header of the
//! backend. See doc/01-Introduction.md and doc/07-Architecture.md.
//!
//! The headers, from the most used:
//! - Drawable.hpp: a shape, its look and how to draw it, in one object.
//! - Buffer.hpp, Texture.hpp: data and pictures on the card.
//! - Shader.hpp, Pipeline.hpp, RenderState.hpp: how to draw.
//! - RenderPass.hpp, Framebuffer.hpp, Draw.hpp: where and what to draw.
//! - ComputeProgram.hpp: calculations on the card.
//! - UniformBlock.hpp, Std140.hpp: values shared by several shaders.
//! - Device.hpp, Errors.hpp, Statistics.hpp: start, errors, counters.
//! - VertexLayout.hpp, ProgramReflection.hpp, FieldReflection.hpp,
//!   PixelFormat.hpp, Enums.hpp, Handle.hpp, DirtyRange.hpp: the vocabulary.
// ****************************************************************************

#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/ComputeProgram.hpp"
#include "Compages/GPU/Device.hpp"
#include "Compages/GPU/DirtyRange.hpp"
#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/Drawable.hpp"
#include "Compages/GPU/Enums.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/FieldReflection.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Handle.hpp"
#include "Compages/GPU/PingPong.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/PixelFormat.hpp"
#include "Compages/GPU/ProgramReflection.hpp"
#include "Compages/GPU/RenderPass.hpp"
#include "Compages/GPU/RenderState.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Statistics.hpp"
#include "Compages/GPU/Std140.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/GPU/UniformBlock.hpp"
#include "Compages/GPU/VertexLayout.hpp"

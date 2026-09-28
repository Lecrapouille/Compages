// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The single header to include to use the GPU layer.
//!
//! \code
//! #include "Compages/GPU/GPU.hpp"
//!
//! // The window is yours to create; the library only wants the loader.
//! if (auto ready = compages::gpu::init(glfwGetProcAddress); !ready)
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
//! backend. See doc/Philosophy.md and doc/Architecture.md.
// ****************************************************************************

#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Compute.hpp"
#include "Compages/GPU/Core/DirtyRange.hpp"
#include "Compages/GPU/Core/Enums.hpp"
#include "Compages/GPU/Core/FieldReflection.hpp"
#include "Compages/GPU/Core/Handle.hpp"
#include "Compages/GPU/Core/Layout.hpp"
#include "Compages/GPU/Core/PixelFormat.hpp"
#include "Compages/GPU/Core/Reflection.hpp"
#include "Compages/GPU/Core/RenderState.hpp"
#include "Compages/GPU/Core/Std140.hpp"
#include "Compages/GPU/Device.hpp"
#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/Drawable.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/RenderPass.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Statistics.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/GPU/UniformBlock.hpp"

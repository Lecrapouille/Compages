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
// OpenGLCppWrapper is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================
//
// This file is a derived work of https://github.com/glumpy/glumpy
//
// Copyright (c) 2009-2016 Nicolas P. Rougier. All rights reserved.
// Distributed under the (new) BSD License.
//=====================================================================

#ifndef OPENGLCPPWRAPPER_GLPROGRAM_HPP
#define OPENGLCPPWRAPPER_GLPROGRAM_HPP

#include "OpenGL/Context/OpenGL.hpp"
#include "OpenGL/Shaders/Shaders.hpp"
#include "OpenGL/Variables/Attribute.hpp"
#include "OpenGL/Variables/Samplers.hpp"
#include "OpenGL/Variables/Uniform.hpp"
#include <map>

class GLVAO;

// *****************************************************************************
//! \brief This class allows to compile shader code (vertex, fragment, geometry)
//! written in GLSL (OpenGL Shading Language) and track their variables (named
//! uniforms and attributes) which then allows to create VBOs and textures when
//! and instance of VAO is bound to the GLProgram.
// *****************************************************************************
class GLProgram: public GLObject<GLenum>
{
    friend class GLVAO;

    //! \brief Memorize GLAttributes, GLUniforms, GLSamplers.
    using Attributes = std::map<std::string, std::unique_ptr<GLAttribute>>;
    using Uniforms = std::map<std::string, std::unique_ptr<GLLocation>>;
    using Samplers = std::map<std::string, std::unique_ptr<GLSampler>>;

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor. Give a name to the instance. This constructor makes
    //! no other actions.
    //!
    //! \param[in] name the name of this instance (used for debug and traces).
    //--------------------------------------------------------------------------
    GLProgram(std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Destructor. Clear list of shaders, list of uniforms and list of
    //! attributes. Unbound last VAO. Clear errors.
    //--------------------------------------------------------------------------
    virtual ~GLProgram();

    //--------------------------------------------------------------------------
    //! \brief Compile a vertex shader and a fragment shader and populate lists
    //! of uniforms and attributes.
    //!
    //! \note: this method is equivalent to the method GLProgram::begin() but
    //! with a more explicit name and return the status of the compilation.
    //!
    //! \return true if the program has been successfully compiled, else return
    //! false. A internal error message is created in case of error that can be
    //! get with strerror().
    //--------------------------------------------------------------------------
    bool compile(GLVertexShader& vertex, GLFragmentShader& fragment);

    //--------------------------------------------------------------------------
    //! \brief Compile a vertex shader, a fragment and geometry shader and
    //! populate list of uniforms and attributes.
    //!
    //! \note: this method is equivalent to the method GLObject::begin() but
    //! with a more explicit name and return the status of the compilation.
    //!
    //! \return true if the program has been successfully compiled, else return
    //! false. A internal error message is created in case of error that can be
    //! get with strerror().
    //--------------------------------------------------------------------------
    bool compile(GLVertexShader& vertex,
                 GLFragmentShader& fragment,
                 GLGeometryShader& geometry);

    //--------------------------------------------------------------------------
    //! \brief Check if attached shaders (passed to attachShaders() method) have
    //! been compiled and linked with success.
    //!
    //! \return true if shaders have been compiled and linked successfuly, else
    //! return false.
    //--------------------------------------------------------------------------
    inline bool compiled() const
    {
        return !m_need_setup;
    }

    //--------------------------------------------------------------------------
    //! \brief Bind VAO with this GLProgram instance.
    //!
    //! Two cases:
    //!   - If it's the first time that the VAO is bound to the GLProgram then
    //!     the VAO has its internal list of VBOs and textures populated. They
    //!     are referred by the variable name used in the GLSL code. VBOs are
    //!     pre-allocated with the number of elements (either set from the by
    //!     GLProgram constructor or by the method bind()) but no data are
    //!     filled, this is the job of the developper to do it explicitly.
    //!
    //!   - Else, if the VAO was bound previously with this instance of program,
    //!     nothing is made (list are not created/updated).
    //!
    //!   - Else, if the VAO was bound previously with another instance of
    //!     program, this is considered as an error and the method return false.
    //!
    //! \note A VAO bound to a GLProgram cannot be bound to a another
    //! GLProgram. This case is refused to avoid to the developer to have a
    //! silent error with an unexpected behavior.
    //!
    //! \param[in] vao the VAO to be bound with this instance of GLProgram.
    //!
    //! \return true if the GLVAO has been bound. If bounding cannot be made
    //! returns false (for one of the following reasons: the GLProgram cannot be
    //! compiled (syntax errors in shader code) or if the VOA has already been
    //! bound by another GLProgram (incompatibility). Call strerror() to get the
    //! error message.
    //--------------------------------------------------------------------------
    bool bind(GLVAO& vao);

    //--------------------------------------------------------------------------
    //! \brief Return all error messages (concatenated by '\\n' char) produced
    //! either during the shader compilation or by an other event.
    //!
    //! \note Once this method as been called the error message is
    //! automatically cleared.
    //!
    //! \return the error message (the message is empty if no error have
    //! occurred).
    //--------------------------------------------------------------------------
    std::string strerror();

    //--------------------------------------------------------------------------
    //! \brief Return the list of shaders that failed to be compiled (if
    //! happened). This is method is mainly used for debug purpose.
    //!
    //! \note if no shader have been attached to a GLProgram or no compilation
    //! error happened or no compilation have been made this method will return
    //! an empty list.
    //!
    //! \param[in,out] list the list where to insert shaders.
    //! \param[in] if the list has to be cleared before being filled.
    //!
    //! \return the number of inserted elements.
    //--------------------------------------------------------------------------
    size_t getFailedShaders(std::vector<std::string>& list,
                            bool const clear = true) const;

    //--------------------------------------------------------------------------
    //! \brief Return the list of uniform names. This is method is mainly used
    //! for debug purpose.
    //!
    //! \note if no shader have been attached to a GLProgram or compilation
    //! error happened or no compilation have been made or the shader code did
    //! not use uniforms this method will return an empty list.
    //!
    //! \param[in,out] list the list where to insert attributes names.
    //! \param[in] if the list has to be cleared before being filled.
    //!
    //! \return the number of inserted elements.
    //--------------------------------------------------------------------------
    size_t getUniformNames(std::vector<std::string>& list,
                           bool const clear = true) const;

    //--------------------------------------------------------------------------
    //! \brief Return the list of attributes names. This is method is mainly
    //! used for debug purpose.
    //!
    //! \note if no shader have been attached to a GLProgram or compilation
    //! error happened or no compilation have been made this method will return
    //! an empty list.
    //!
    //! \param[in,out] list the list where to insert attributes names.
    //! \param[in] if the list has to be cleared before being filled.
    //!
    //! \return the number of inserted elements.
    //--------------------------------------------------------------------------
    size_t getAttributeNames(std::vector<std::string>& list,
                             bool const clear = true) const;

    //--------------------------------------------------------------------------
    //! \brief Return the list of texture names. This is method is mainly
    //! used for debug purpose.
    //!
    //! \note Do not confuse samplers with textures. In this method, we do not
    //! refer to the file name with jpeg, png or bmp ... extensions but to the
    //! GLSL uniform name (used inside shader code sources).
    //!
    //! \note if no shader have been attached to a GLProgram or compilation
    //! error happened or no compilation have been made or the shader code did
    //! not use sampler, this method will return an empty list.
    //!
    //! \param[in,out] list the list where to insert sampler names.
    //! \param[in] if the list has to be cleared before being filled.
    //!
    //! \return the number of inserted elements.
    //--------------------------------------------------------------------------
    size_t getSamplerNames(std::vector<std::string>& list,
                           bool const clear = true) const;

    //--------------------------------------------------------------------------
    //! \brief Return the list of attributes. Only reachable by VAO class.
    //--------------------------------------------------------------------------
    Attributes const& attributes() const
    {
        return m_attributes;
    }

    //--------------------------------------------------------------------------
    //! \brief Check the presence of the GLAttribute<T>
    //--------------------------------------------------------------------------
    bool hasAttribute(const char* name) const
    {
        auto const it = m_attributes.find(name);
        if (it == m_attributes.end())
            return false;
        return dynamic_cast<GLAttribute const*>(it->second.get()) != nullptr;
    }

    //--------------------------------------------------------------------------
    //! \brief  Return the list of samplers. Only reachable by VAO class.
    //--------------------------------------------------------------------------
    Samplers const& samplers() const
    {
        return m_samplers;
    }

    //--------------------------------------------------------------------------
    //! \brief Check the presence of the GLSampler<T>
    //--------------------------------------------------------------------------
    bool hasSampler(const char* name) const
    {
        auto const it = m_samplers.find(name);
        if (it == m_samplers.end())
            return false;
        return dynamic_cast<GLSampler const*>(it->second.get()) != nullptr;
    }

    //--------------------------------------------------------------------------
    //! \brief Check if program has a non empty list of shader uniforms.
    //!
    //! \return the number of uniforms.
    //--------------------------------------------------------------------------
    Uniforms const& uniforms() const
    {
        return m_uniforms;
    }

    //--------------------------------------------------------------------------
    //! \brief Check the presence of the uniform
    //--------------------------------------------------------------------------
    template <class T>
    bool hasUniform(const char* name) const
    {
        auto const it = m_uniforms.find(name);
        if (it == m_uniforms.end())
            return false;
        return dynamic_cast<GLUniform<T> const*>(it->second.get()) != nullptr;
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform float 4x4 matrix. This
    //! method wraps the \a uniform() method hiding the misery of the template.
    //--------------------------------------------------------------------------
    inline Matrix44f& matrix44f(const char* name)
    {
        return uniform<Matrix44f>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform float 3x3 matrix. This
    //! method wraps the \a uniform() method hiding the misery of the template.
    //--------------------------------------------------------------------------
    inline Matrix33f& matrix33f(const char* name)
    {
        return uniform<Matrix33f>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform float 2x2 matrix.
    //! This method wraps the \a uniform() method hiding the misery of
    //! the template.
    //--------------------------------------------------------------------------
    inline Matrix22f& matrix22f(const char* name)
    {
        return uniform<Matrix22f>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform float 4D vector.
    //! This method wraps the \a uniform() method hiding the misery of
    //! the template.
    //--------------------------------------------------------------------------
    inline Vector4f& vector4f(const char* name)
    {
        return uniform<Vector4f>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform float 3D vector.
    //! This method wraps the \a uniform() method hiding the misery of
    //! the template.
    //--------------------------------------------------------------------------
    inline Vector3f& vector3f(const char* name)
    {
        return uniform<Vector3f>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform float 2D vector.
    //! This method wraps the \a uniform() method hiding the misery of
    //! the template.
    //--------------------------------------------------------------------------
    inline Vector2f& vector2f(const char* name)
    {
        return uniform<Vector2f>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform float scalar.
    //! This method wraps the \a uniform() method hiding the misery of
    //! the template.
    //--------------------------------------------------------------------------
    inline float& scalarf(const char* name)
    {
        return uniform<float>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform integer 4D vector.
    //! This method wraps the \a uniform() method hiding the misery of
    //! the template.
    //--------------------------------------------------------------------------
    inline Vector4i& vector4i(const char* name)
    {
        return uniform<Vector4i>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform integer 3D vector.
    //! This method wraps the \a uniform() method hiding the misery of
    //! the template.
    //--------------------------------------------------------------------------
    inline Vector3i& vector3i(const char* name)
    {
        return uniform<Vector3i>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform integer 2D vector.
    //! This method wraps the \a uniform() method hiding the misery of
    //! the template.
    //--------------------------------------------------------------------------
    inline Vector2i& vector2i(const char* name)
    {
        return uniform<Vector2i>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and return the shader uniform integer scalar.
    //! This method wraps the \a uniform() method hiding the misery of
    //! the template.
    //--------------------------------------------------------------------------
    inline int& scalar(const char* name)
    {
        return uniform<int>(name);
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Locate the uniform variable by its name and its type T.
    //! \return the uniform instance if found else throw an exception.
    //! \throw OpenGLException if the uniform does not exist or bad T type
    //! param.
    //--------------------------------------------------------------------------
    template <class T>
    GLUniform<T>& uniform(const char* name)
    {
        if (compiled())
        {
            if (auto it = m_uniforms.find(name); it != m_uniforms.end())
            {
                if (auto uniform =
                        dynamic_cast<GLUniform<T>*>(it->second.get());
                    uniform != nullptr)
                {
                    return *uniform;
                }

                throw GL::Exception("GLUniform " + std::string(name) +
                                    " exists but has wrong template type");
            }
            throw GL::Exception("GLUniform " + std::string(name) +
                                " does not exist");
        }
        else
        {
            // The API allows the user to define uniforms before compiling the
            // GLProgram.
            if (m_uniforms.find(name) == m_uniforms.end())
            {
                createUniform<T>(name);
            }

            auto uniform = dynamic_cast<GLUniform<T>*>(m_uniforms[name].get());
            return *uniform;
        }
    }

    //--------------------------------------------------------------------------
    //! \brief From C++ type return the OpenGL enum (ie float => GL_FLOAT).
    //--------------------------------------------------------------------------
    template <class T>
    inline GLenum getGLAttributeType();
    template <class T>
    inline GLenum getGLUniformType();
    template <class T>
    inline GLint getGLDimension();

    //--------------------------------------------------------------------------
    //! \brief Do the shader compilation
    //--------------------------------------------------------------------------
    bool compile();

    //--------------------------------------------------------------------------
    //! \brief Concat the last error to the list of errors
    //--------------------------------------------------------------------------
    void concatError(std::string const& msg);

    //--------------------------------------------------------------------------
    //! \brief Create a new OpenGL program. Compile and link shaders bound to
    //! this GLProgram instance.
    //!
    //! \return false is compilation succeeded (indicating setup has not to be
    //! redone). Return true if an error occurred (errors in the code source of
    //! shaders).
    //--------------------------------------------------------------------------
    virtual bool onCreate() override;

    //--------------------------------------------------------------------------
    //! \brief Activate in OpenGL the program, its attributes, its uniforms and
    //! samplers. A VAO shall be bound else nothing is made.
    //--------------------------------------------------------------------------
    virtual void onActivate() override;

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual bool onSetup() override;

    //--------------------------------------------------------------------------
    //! \brief Dummy method. Nothing is made.
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override;

    //--------------------------------------------------------------------------
    //! \brief Deactivate the OpenGL program, its uniform, attributes, samplers.
    //!
    //! \note A program can be deactivated if and only if shaders have
    //! been loaded into a program (else nothing is done).
    //--------------------------------------------------------------------------
    virtual void onDeactivate() override;

    //--------------------------------------------------------------------------
    //! \brief Once program is no longer used, release it from the GPU
    //! memory. Can be used to abort the shader.
    //--------------------------------------------------------------------------
    virtual void onRelease() override;

    //--------------------------------------------------------------------------
    //! \brief Check if the shaders have been successfully linked.
    //!
    //! \return true if case of success, else return false.
    //--------------------------------------------------------------------------
    bool checkLinkageStatus(GLuint handle);

    //--------------------------------------------------------------------------
    //! \brief Release the GPU memory by removing compiled shaders.
    //--------------------------------------------------------------------------
    void detachAllShaders();

    //--------------------------------------------------------------------------
    //! \brief Create uniform, texture sampler and attribute lists.
    //--------------------------------------------------------------------------
    bool generateAttributesAndUniforms();

    //--------------------------------------------------------------------------
    //! \brief General method for creating uniform instances.
    //--------------------------------------------------------------------------
    bool storeUniformOrSampler(GLenum type, const char* name);
    void storeAttribute(GLenum type, const char* name);

    //--------------------------------------------------------------------------
    //! \brief Specific method for creating uniform instances.
    //--------------------------------------------------------------------------
    template <class T>
    inline void createUniform(const char* name)
    {
        m_uniforms[name] = std::make_unique<GLUniform<T>>(
            name, getGLDimension<T>(), getGLUniformType<T>(), handle());
    }

    //--------------------------------------------------------------------------
    //! \brief Specific method for creating uniform instances.
    //--------------------------------------------------------------------------
    template <class T>
    inline bool updateOrCreateUniform(const char* name)
    {
        // Try to insert new GLUniform
        auto const& it = m_uniforms.insert(std::make_pair(
            name,
            std::make_unique<GLUniform<T>>(
                name, getGLDimension<T>(), getGLUniformType<T>(), handle())));

        // Already stored ? This is fine since the API allows creating uniform
        // before compiling the shader.
        if (!it.second)
        {
            // Well typed ?
            GLUniform<T>* uniform =
                dynamic_cast<GLUniform<T>*>(it.first->second.get());
            if (uniform != nullptr)
            {
                uniform->m_size = getGLDimension<T>();
                uniform->m_target = getGLUniformType<T>();
                uniform->m_program = handle();
            }
            else
            {
                // Wrong type ?
                std::string msg =
                    "GLUniform " + std::string(name) +
                    " mismatch type:"
                    " shader type is different from the one you have"
                    " created before compiling the shader code";
                concatError(msg);
                return false;
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------
    //! \brief Create texture sampler instances.
    //--------------------------------------------------------------------------
    template <class T>
    inline void createSampler(const char* name)
    {
        m_samplers[name] =
            std::make_unique<T>(name, m_samplers.size(), handle());
    }

    //--------------------------------------------------------------------------
    //! \brief Create Attribute instances
    //--------------------------------------------------------------------------
    template <class T>
    inline void createAttribute(const char* name)
    {
        m_attributes[name] = std::make_unique<GLAttribute>(
            name, getGLDimension<T>(), getGLAttributeType<T>(), handle());
    }

private:

    //! \brief Hold temporary vertex, fragment, geometry shaders that need to be
    //! compiled. Once compiled this list is erased to avoid refering to dead
    //! external variables.
    std::vector<GLShader*> m_shaders;
    //! \brief Hold the localization of shader attributes.
    Attributes m_attributes;
    //! \brief Hold the localization of shader uniforms.
    Uniforms m_uniforms;
    //! \brief Hold the localisation of uniform texture sampler.
    Samplers m_samplers;
    //! \brief
    std::vector<std::string> m_failedShaders;
    //! \brief Memorize all errors
    std::string m_error;
};

template <>
inline GLenum GLProgram::getGLAttributeType<float>()
{
    return GL_FLOAT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Vector2f>()
{
    return GL_FLOAT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Vector3f>()
{
    return GL_FLOAT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Vector4f>()
{
    return GL_FLOAT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<int>()
{
    return GL_INT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Vector2i>()
{
    return GL_INT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Vector3i>()
{
    return GL_INT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Vector4i>()
{
    return GL_INT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<unsigned int>()
{
    return GL_UNSIGNED_INT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Vector2u>()
{
    return GL_UNSIGNED_INT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Vector3u>()
{
    return GL_UNSIGNED_INT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Vector4u>()
{
    return GL_UNSIGNED_INT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Matrix22f>()
{
    return GL_FLOAT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Matrix33f>()
{
    return GL_FLOAT;
}
template <>
inline GLenum GLProgram::getGLAttributeType<Matrix44f>()
{
    return GL_FLOAT;
}

template <>
inline GLenum GLProgram::getGLUniformType<float>()
{
    return GL_FLOAT;
}
template <>
inline GLenum GLProgram::getGLUniformType<Vector2f>()
{
    return GL_FLOAT_VEC2;
}
template <>
inline GLenum GLProgram::getGLUniformType<Vector3f>()
{
    return GL_FLOAT_VEC3;
}
template <>
inline GLenum GLProgram::getGLUniformType<Vector4f>()
{
    return GL_FLOAT_VEC4;
}
template <>
inline GLenum GLProgram::getGLUniformType<Matrix22f>()
{
    return GL_FLOAT_MAT2;
}
template <>
inline GLenum GLProgram::getGLUniformType<Matrix33f>()
{
    return GL_FLOAT_MAT3;
}
template <>
inline GLenum GLProgram::getGLUniformType<Matrix44f>()
{
    return GL_FLOAT_MAT4;
}
template <>
inline GLenum GLProgram::getGLUniformType<int>()
{
    return GL_INT;
}
template <>
inline GLenum GLProgram::getGLUniformType<Vector2i>()
{
    return GL_INT_VEC2;
}
template <>
inline GLenum GLProgram::getGLUniformType<Vector3i>()
{
    return GL_INT_VEC3;
}
template <>
inline GLenum GLProgram::getGLUniformType<Vector4i>()
{
    return GL_INT_VEC4;
}
template <>
inline GLenum GLProgram::getGLUniformType<unsigned int>()
{
    return GL_UNSIGNED_INT;
}
template <>
inline GLenum GLProgram::getGLUniformType<Vector2u>()
{
    return GL_UNSIGNED_INT_VEC2;
}
template <>
inline GLenum GLProgram::getGLUniformType<Vector3u>()
{
    return GL_UNSIGNED_INT_VEC3;
}
template <>
inline GLenum GLProgram::getGLUniformType<Vector4u>()
{
    return GL_UNSIGNED_INT_VEC4;
}

template <>
inline GLint GLProgram::getGLDimension<float>()
{
    return 1;
}
template <>
inline GLint GLProgram::getGLDimension<Vector2f>()
{
    return 2;
}
template <>
inline GLint GLProgram::getGLDimension<Vector3f>()
{
    return 3;
}
template <>
inline GLint GLProgram::getGLDimension<Vector4f>()
{
    return 4;
}
template <>
inline GLint GLProgram::getGLDimension<Matrix22f>()
{
    return 4;
}
template <>
inline GLint GLProgram::getGLDimension<Matrix33f>()
{
    return 9;
}
template <>
inline GLint GLProgram::getGLDimension<Matrix44f>()
{
    return 16;
}
template <>
inline GLint GLProgram::getGLDimension<int>()
{
    return 1;
}
template <>
inline GLint GLProgram::getGLDimension<Vector2i>()
{
    return 2;
}
template <>
inline GLint GLProgram::getGLDimension<Vector3i>()
{
    return 3;
}
template <>
inline GLint GLProgram::getGLDimension<Vector4i>()
{
    return 4;
}
template <>
inline GLint GLProgram::getGLDimension<unsigned int>()
{
    return 1;
}
template <>
inline GLint GLProgram::getGLDimension<Vector2u>()
{
    return 2;
}
template <>
inline GLint GLProgram::getGLDimension<Vector3u>()
{
    return 3;
}
template <>
inline GLint GLProgram::getGLDimension<Vector4u>()
{
    return 4;
}

#endif // OPENGLCPPWRAPPER_GLPROGRAM_HPP

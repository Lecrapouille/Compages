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

#ifndef OPENGLCPPWRAPPER_TRANSFORMABLE_HPP
#define OPENGLCPPWRAPPER_TRANSFORMABLE_HPP

#include "Math/Quaternion.hpp"
#include "Math/Transformation.hpp"

// *****************************************************************************
//! \brief The coordinate space in which to operate.
// *****************************************************************************
enum class Space
{
    //! \brief Use \c World to transform a GameObject the world coordinates.
    World,
    //! \brief Use \c Self to transform a GameObject using its own coordinates.
    Self
};

// *****************************************************************************
//! \brief a Transformable defines a 4x4 transformation matrix from a given
//! translation, a given rotation and a given scale factor. This class allows an
//! object to move inside a 2D-world (if n == 2U) or 3D-world (if n == 3U).
//! The returned matrix will be given to OpenGL shader for rendering the object
//! inside the world.
//!
//! \warning Note that all matrices are transposed to be directly usable by
//!   OpenGL (since OpenGL matrices are column-major. Beware of option passed
//!   to glUniformMatrix4fv that should be set to GL_FALSE). As consequence:
//!   transpose(A * B) = transpos(B) * transpose(A).
// *****************************************************************************
template <typename T, size_t n>
class Transformable
{
public:

    //--------------------------------------------------------------------------
    //! \brief Empty constructor. Init position and origin to 0.
    //! Init scale to 1. Init to transformation to identity matrix.
    //--------------------------------------------------------------------------
    Transformable()
        : m_origin(maths::zero<T>()),
          m_position(maths::zero<T>()),
          m_scale(maths::one<T>()),
          m_local_scaling(maths::one<T>()),
          m_transform(matrix::Identity),
          m_inverse_transform(matrix::Identity),
          m_transform_needs_update(false),
          m_inverse_trans_needs_update(false)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Restore states to default. Init position and origin to 0.
    //! Init scale to 1. Init to transformation to identity matrix.
    //--------------------------------------------------------------------------
    void reset()
    {
        m_origin = Vector<T, n>::ZERO;
        m_position = Vector<T, n>::ZERO;
        m_orientation = Quat<T>();
        m_scale = Vector<T, n>::ONE;
        m_local_scaling = Vector<T, n>::ONE;
        matrix::identity(m_transform);
        matrix::identity(m_inverse_transform);
        m_transform_needs_update = false;
        m_inverse_trans_needs_update = false;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the right vector.
    //--------------------------------------------------------------------------
    const Vector<T, n> right() const
    {
        return Vector<T, n>(rotation()[0]);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the up vector.
    //--------------------------------------------------------------------------
    const Vector<T, n> up() const
    {
        return Vector<T, n>(rotation()[1]);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the forward vector.
    //--------------------------------------------------------------------------
    const Vector<T, n> forward() const
    {
        return Vector<T, n>(rotation()[2]);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the direction.
    //--------------------------------------------------------------------------
    Vector<T, n> direction()
    {
        Matrix<T, n + 1u, n + 1u> m = matrix();
        Vector<T, n> v(-m[0][2], -m[1][2], -m[2][2]);
        return vector::normalize(v);
    }

    //--------------------------------------------------------------------------
    //! \brief Set the origin of the object relative to the world origin.
    //! \param[in] origin: World coordinates.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& origin(Vector<T, n> const& origin)
    {
        m_origin = origin;
        m_transform_needs_update = true;

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Get the origin of the object relative to the world origin.
    //! \return World coordinates.
    //--------------------------------------------------------------------------
    inline Vector<T, n> const& origin() const
    {
        return m_origin;
    }

    //--------------------------------------------------------------------------
    //! \brief Set the position of the object relative to the world origin.
    //! \param[in] position: World coordinates.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& position(Vector<T, n> const& position)
    {
        m_position = position;
        m_transform_needs_update = true;

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Get the position of the object (relative to the world origin).
    //! \return World coordinates.
    //--------------------------------------------------------------------------
    inline Vector<T, n> const& position() const
    {
        return m_position;
    }

    //--------------------------------------------------------------------------
    //! \brief Set the position of the object (relative to its origin).
    //! \param[in] position: World coordinates.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& local_position(Vector<T, n> const& position)
    {
        m_position = position + m_origin;
        m_transform_needs_update = true;

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Get the position of the object (relative to its origin).
    //! \return Local coordinates.
    //--------------------------------------------------------------------------
    inline Vector<T, n> local_position() const
    {
        return m_position - m_origin;
    }

    //--------------------------------------------------------------------------
    //! \brief Move the object by a given offset.
    //! \note Do the same job than move() or displace().
    //! \param[in] offset: relative displacement. How much we want to move the
    //! object in which direction.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& translate(Vector<T, n> const& offset,
                                          Space relativeTo = Space::World)
    {
        if (relativeTo == Space::World)
        {
            m_position += offset;
        }
        else
        {
            m_position += (m_orientation * offset);
        }

        m_transform_needs_update = true;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Move the object by a given offset to the right (positive offset)
    //! or to the left (negative offset).
    //! \param[in] offset: relative displacement. How much we want to move the
    //! object along its right or left direction.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& moveRight(T const& offset)
    {
        m_position += offset * right();
        m_transform_needs_update = true;

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Move up the object by a given offset (positive offset)
    //! or to move down (negative offset).
    //! \param[in] offset: relative displacement. How much we want to move the
    //! object along its up or down direction.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& moveUp(T const& offset)
    {
        m_position += offset * up();
        m_transform_needs_update = true;

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Move forward the object by a given offset (positive offset)
    //! or to move backward (negative offset).
    //! \param[in] offset: relative displacement. How much we want to move the
    //! object along its forward or backward direction.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& moveForward(T const& offset)
    {
        m_position += offset * forward();
        m_transform_needs_update = true;

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the translation matrix
    //--------------------------------------------------------------------------
    Matrix<T, n + 1u, n + 1u> translation()
    {
        Matrix<T, n + 1u, n + 1u> I(matrix::Identity);
        m_transform = matrix::translate(I, m_position);
        return m_transform;
    }

    //--------------------------------------------------------------------------
    //! \brief Set the absolute scale factor of the object.
    //! \param[in] offset: relative displacement.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& scaling(Vector<T, n> const& scale)
    {
        m_scale = scale;
        m_transform_needs_update = true;

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Get the absolute scale factors of the object.
    //--------------------------------------------------------------------------
    inline Vector<T, n> const& scaling() const
    {
        return m_scale;
    }

    //--------------------------------------------------------------------------
    //! \brief Relative scaling of the object.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& scale(Vector<T, n> const& factor)
    {
        m_scale *= factor;
        m_transform_needs_update = true;

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Set a local scaling of the object. By local we mean that
    //! will not impact children (they will not be scaled).
    //! \note for scaling children use methods Transformable::scale or
    //! Transformable::scaleFactor.
    //--------------------------------------------------------------------------
    inline Transformable<T, n>& localScale(Vector<T, n> const& scale)
    {
        m_local_scaling = scale;

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Get the local scaling.
    //--------------------------------------------------------------------------
    inline Vector<T, n> const& localScale() const
    {
        return m_local_scaling;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the rotation matrix
    //--------------------------------------------------------------------------
    inline Matrix<T, n + 1u, n + 1u> rotation() const
    {
        return m_orientation.toMatrix();
    }

    //--------------------------------------------------------------------------
    //! \brief Set the new attitude (new orientation).
    //--------------------------------------------------------------------------
    inline void attitude(Quatf const& q)
    {
        m_orientation = q;
        m_transform_needs_update = true;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the quaternion orientation
    //--------------------------------------------------------------------------
    Quatf const& attitude() const
    {
        return m_orientation;
    }

    //--------------------------------------------------------------------------
    //! \brief Set the absolute orientation of the object.
    //! \param angle in radian.
    //--------------------------------------------------------------------------
    Transformable<T, n>& rotate(units::angle::radian_t const angle,
                                Vector<T, n> const& axis,
                                Space relativeTo = Space::Self)
    {
        return rotate(Quatf::fromAngleAxis(angle, axis), relativeTo);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    Transformable<T, n>& rotate(Quat<T> const& q,
                                Space relativeTo = Space::Self)
    {
        if (relativeTo == Space::Self)
        {
            m_orientation = m_orientation * q;
        }
        else
        {
            m_orientation = q * m_orientation;
        }
        m_orientation.normalize();

        m_transform_needs_update = true;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Local rotation over its right axis.
    //--------------------------------------------------------------------------
    Transformable<T, n>& pitch(units::angle::radian_t const angle,
                               Space relativeTo = Space::Self)
    {
        return rotate(angle, right(), relativeTo);
    }

    //--------------------------------------------------------------------------
    //! \brief Local rotation over its up axis.
    //--------------------------------------------------------------------------
    Transformable<T, n>& yaw(units::angle::radian_t const angle,
                             Space relativeTo = Space::Self)
    {
        return rotate(angle, up(), relativeTo);
    }

    //--------------------------------------------------------------------------
    //! \brief Local rotation over its forward axis.
    //--------------------------------------------------------------------------
    Transformable<T, n>& roll(units::angle::radian_t const angle,
                              Space relativeTo = Space::Self)
    {
        return rotate(angle, forward(), relativeTo);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //!
    //! When multiplied by the LookAt view matrix, the world-space vectors are
    //! rotated (brought) into the camera's view while the camera's orientation
    //! is kept in place. So an actual rotation of the camera by 45 degress to
    //! the right is achieved with a matrix which applies a 45 degree rotation
    //! to the left to all the world-space vertices.
    //!
    //! Correct rotation can be done in two ways:
    //!  - Calculate the inverse of the lookAt matrix and multiply the
    //!    world-space direction vectors by this rotation matrix.
    //!  - (more efficient) Convert the LookAt matrix into a quaternion and
    //!    conjugate it instead of applying glm::inverse, since the result is a
    //!    unit quat and for such quats the inverse is equal to the conjugate.
    //--------------------------------------------------------------------------
    void
    lookAt(Vector3f const& position, Vector3f const& target, Vector3f const& up)
    {
        m_position = position;
        Matrix44f mat = matrix::lookAt(position, target, up);
        m_orientation = Quatf::fromMatrix(mat).conjugate();
        m_transform_needs_update = true;
    }

    //--------------------------------------------------------------------------
    //! \brief Same than lookAt(position, target, up) but the up direction is
    //! deduced.
    //--------------------------------------------------------------------------
    void lookAt(Vector3f const& position, Vector3f const& target)
    {
        // Get back the up direction
        Vector3f direction = vector::normalize(target - position);
        Vector3f right = vector::cross(direction, Vector3f::UP);
        Vector3f up = vector::cross(right, direction);

        // Same than lookAt(position, target, up)
        m_position = position;
        Matrix44f mat = matrix::lookAt(m_position, target, up);
        m_orientation = Quatf::fromMatrix(mat).conjugate();
        m_transform_needs_update = true;
    }

    //--------------------------------------------------------------------------
    //! \brief Same than lookAt(position, target, up) but the positon is not
    //! changed and the up direction is deduced.
    //--------------------------------------------------------------------------
    void lookAt(Vector3f const& target)
    {
        // Get back the up direction
        Vector3f direction = vector::normalize(target - m_position);
        Vector3f right = vector::cross(direction, Vector3f::UP);
        Vector3f up = vector::cross(right, direction);

        // Same than lookAt(position, target, up)
        Matrix44f mat = matrix::lookAt(m_position, target, up);
        m_orientation = Quatf::fromMatrix(mat).conjugate();
        m_transform_needs_update = true;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the 4x4 transform matrix combining the
    //! position/rotation/scale/origin of the object.
    //!
    //! Operation:
    //!   Transform = TranslationMatrix * RotationMatrix * ScaleMatrix;
    //!   TransformedVector = Transform * OriginalVector;
    //! Note:
    //!   Be careful of operation order: we apply scale first, then
    //!   the rotation then the translation.
    //--------------------------------------------------------------------------
    Matrix<T, n + 1u, n + 1u> const& matrix() // FIXME should be const
    {
        if (m_transform_needs_update)
        {
            Matrix<T, n + 1u, n + 1u> I(matrix::Identity);
            m_transform = matrix::translate(I, m_position - m_origin);
            m_transform = matrix::rotate(
                m_transform, m_orientation.angle(), m_orientation.axis());
            m_transform = matrix::scale(m_transform, m_scale);
            m_transform_needs_update = false;
            m_inverse_trans_needs_update = true;
        }

        return m_transform;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the 4x4 inverse transform matrix.
    //--------------------------------------------------------------------------
    Matrix<T, n + 1u, n + 1u> const& invMatrix()
    {
        if (m_inverse_trans_needs_update)
        {
            m_inverse_transform = matrix::inverse(matrix());
            m_inverse_trans_needs_update = false;
        }
        return m_inverse_transform;
    }

    inline bool modified() const
    {
        return m_transform_needs_update;
    }

protected:

    //! \brief Position of the origin relative to the origin of the world.
    Vector<T, n> m_origin;
    //! \brief Position of the object relative to its origin.
    Vector<T, n> m_position;
    //! \brief Orientation (quaternion).
    Quat<T> m_orientation;
    //! \brief Relative scaling of the object relative to the world.
    Vector<T, n> m_scale;
    //! \brief Scaling factors for the object.
    Vector<T, n> m_local_scaling;
    //! \brief Local transformation
    Matrix<T, n + 1u, n + 1u> m_transform;
    //! \brief Inversed transformation
    Matrix<T, n + 1u, n + 1u> m_inverse_transform;
    //! \brief Shall m_transform need to be computed ?
    bool m_transform_needs_update;
    //! \brief Shall m_inverse_transform need to be computed ?
    bool m_inverse_trans_needs_update;
};

using Transformable3D = Transformable<float, 3u>;

#endif // OPENGLCPPWRAPPER_TRANSFORMABLE_HPP

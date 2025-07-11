//=====================================================================
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:     BSL-1.0
//
//=====================================================================
// Inspired by the POCO project, adapted for the OpenGLCppWrapper project.
// The original POCO code source:
//   https://github.com/pocoproject/poco
// For more informations, see the official website:
//   https://pocoproject.org/index.html
//=====================================================================

#ifndef OPENGLCPPWRAPPER_EXCEPTION_HPP
#define OPENGLCPPWRAPPER_EXCEPTION_HPP

#include "NonCppStd.hpp"
#include <string>
#include <typeinfo>

class BaseException: public std::exception
/// Base class for all exceptions defined in this project.
{
public:

    BaseException(const std::string& msg, int code = 0);
    /// Creates an exception.

    BaseException(const std::string& msg, const std::string& arg, int code = 0);
    /// Creates an exception.

    BaseException(const std::string& msg,
                  const BaseException& nested,
                  int code = 0);
    /// Creates an exception and stores a clone
    /// of the nested exception.

    BaseException(const BaseException& exc);
    /// Copy constructor.

    ~BaseException(); /* throw() */
    /// Destroys the exception and deletes the nested exception.

    BaseException& operator=(const BaseException& exc);
    /// Assignment operator.

    virtual const char* name() const; /* throw() */
    /// Returns a static string describing the exception.

    virtual const char* className() const; /* throw() */
    /// Returns the name of the exception class.

    virtual const char* what() const noexcept override; /* throw() */
    /// Returns a static string describing the exception.
    ///
    /// Same as name(), but for compatibility with std::exception.

    const BaseException* nested() const;
    /// Returns a pointer to the nested exception, or
    /// null if no nested exception exists.

    const std::string& message() const;
    /// Returns the message text.

    int code() const;
    /// Returns the exception code if defined.

    std::string displayText() const;
    /// Returns a string consisting of the
    /// message name and the message text.

    virtual BaseException* clone() const;
    /// Creates an exact copy of the exception.
    ///
    /// The copy can later be thrown again by
    /// invoking rethrow() on it.

    virtual void rethrow() const;
    /// (Re)Throws the exception.
    ///
    /// This is useful for temporarily storing a
    /// copy of an exception (see clone()), then
    /// throwing it again.

protected:

    BaseException(int code = 0);
    /// Standard constructor.

    void message(const std::string& msg);
    /// Sets the message for the exception.

    void extendedMessage(const std::string& arg);
    /// Sets the extended message for the exception.

protected:

    std::string m_msg;
    BaseException* m_pNested;
    int m_code;
};

//
// inlines
//
inline const BaseException* BaseException::nested() const
{
    return m_pNested;
}

inline const std::string& BaseException::message() const
{
    return m_msg;
}

inline void BaseException::message(const std::string& msg)
{
    m_msg = msg;
}

inline int BaseException::code() const
{
    return m_code;
}

//
// Macros for quickly declaring and implementing exception classes.
// Unfortunately, we cannot use a template here because character
// pointers (which we need for specifying the exception name)
// are not allowed as template arguments.
//
#define DECLARE_EXCEPTION_CODE(/*API,*/ CLS, BASE, CODE)                      \
    class /*API*/ CLS: public BASE                                            \
    {                                                                         \
    public:                                                                   \
                                                                              \
        CLS(int code = CODE);                                                 \
        CLS(const std::string& msg, int code = CODE);                         \
        CLS(const std::string& msg, const std::string& arg, int code = CODE); \
        CLS(const std::string& msg,                                           \
            const BaseException& exc,                                         \
            int code = CODE);                                                 \
        CLS(const CLS& exc);                                                  \
        ~CLS(); /* throw() */                                                 \
        CLS& operator=(const CLS& exc);                                       \
        const char* name() const;      /* throw() */                          \
        const char* className() const; /* throw() */                          \
        BaseException* clone() const;                                         \
        void rethrow() const;                                                 \
    };

#define DECLARE_EXCEPTION(/*API,*/ CLS, BASE) \
    DECLARE_EXCEPTION_CODE(/*API,*/ CLS, BASE, 0)

#define IMPLEMENT_EXCEPTION(CLS, BASE, NAME)                             \
    CLS::CLS(int code) : BASE(code) {}                                   \
    CLS::CLS(const std::string& msg, int code) : BASE(msg, code) {}      \
    CLS::CLS(const std::string& msg, const std::string& arg, int code)   \
        : BASE(msg, arg, code)                                           \
    {                                                                    \
    }                                                                    \
    CLS::CLS(const std::string& msg, const BaseException& exc, int code) \
        : BASE(msg, exc, code)                                           \
    {                                                                    \
    }                                                                    \
    CLS::CLS(const CLS& exc) : BASE(exc) {}                              \
    CLS::~CLS() /* throw() */ {}                                         \
    CLS& CLS::operator=(const CLS& exc)                                  \
    {                                                                    \
        BASE::operator=(exc);                                            \
        return *this;                                                    \
    }                                                                    \
    const char* CLS::name() const /* throw() */                          \
    {                                                                    \
        return NAME;                                                     \
    }                                                                    \
    const char* CLS::className() const /* throw() */                     \
    {                                                                    \
        return typeid(*this).name();                                     \
    }                                                                    \
    BaseException* CLS::clone() const                                    \
    {                                                                    \
        return new CLS(*this);                                           \
    }                                                                    \
    NORETURN void CLS::rethrow() const                                   \
    {                                                                    \
        throw *this;                                                     \
    }

#endif // OPENGLCPPWRAPPER_EXCEPTION_HPP

// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:     BSL-1.0
//
// NOTE: This file has been adapted for the OpenGLCppWrapper project.
// For more informations, see official Poco webpage:
// https://pocoproject.org/index.html

#include "Exception.hpp"

BaseException::BaseException(int code) : m_pNested(0), m_code(code) {}

BaseException::BaseException(const std::string& msg, int code)
    : m_msg(msg), m_pNested(0), m_code(code)
{
}

BaseException::BaseException(const std::string& msg,
                             const std::string& arg,
                             int code)
    : m_msg(msg), m_pNested(0), m_code(code)
{
    if (!arg.empty())
    {
        m_msg.append(": ");
        m_msg.append(arg);
    }
}

BaseException::BaseException(const std::string& msg,
                             const BaseException& nested,
                             int code)
    : m_msg(msg), m_pNested(nested.clone()), m_code(code)
{
}

BaseException::BaseException(const BaseException& exc)
    : std::exception(exc), m_msg(exc.m_msg), m_code(exc.m_code)
{
    m_pNested = exc.m_pNested ? exc.m_pNested->clone() : 0;
}

BaseException::~BaseException() /* throw() */
{
    delete m_pNested;
}

BaseException& BaseException::operator=(const BaseException& exc)
{
    if (&exc != this)
    {
        BaseException* newPNested = exc.m_pNested ? exc.m_pNested->clone() : 0;
        delete m_pNested;
        m_msg = exc.m_msg;
        m_pNested = newPNested;
        m_code = exc.m_code;
    }
    return *this;
}

const char* BaseException::name() const /* throw() */
{
    return "BaseException";
}

const char* BaseException::className() const /* throw() */
{
    return typeid(*this).name();
}

const char* BaseException::what() const noexcept /* throw() */
{
    return name();
}

std::string BaseException::displayText() const
{
    std::string txt = name();
    if (!m_msg.empty())
    {
        txt.append(": ");
        txt.append(m_msg);
    }
    return txt;
}

void BaseException::extendedMessage(const std::string& arg)
{
    if (!arg.empty())
    {
        if (!m_msg.empty())
            m_msg.append(": ");
        m_msg.append(arg);
    }
}

BaseException* BaseException::clone() const
{
    return new BaseException(*this);
}

NORETURN void BaseException::rethrow() const
{
    throw *this;
}

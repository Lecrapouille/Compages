#include "foo.hpp"
#include "project_info.hpp"
#include <iostream>

int main()
{
    std::cout << "Welcome to your project " << project::info::name
              << " version " << project::info::version::major << '.'
              << project::info::version::minor << " compiled in "
              << (project::info::compilation::mode ==
                          project::info::compilation::debug
                      ? "debug"
                      : (project::info::compilation::mode ==
                             project::info::compilation::release
                         ? "release"
                         : "normal"))
              << " mode " << std::endl
              << " application " << project::info::application::name
              << " summary: '" << project::info::application::summary << "'"
              << std::endl
              << "Your code was git cloned on branch "
              << project::info::git::branch << " SHA1 "
              << project::info::git::sha1 << std::endl;
    return foo(1, 2);
}

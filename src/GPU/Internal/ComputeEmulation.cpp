// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Internal/ComputeEmulation.hpp"
#include "GPU/Internal/GlslText.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

namespace compages::gpu::detail
{

namespace
{

// ****************************************************************************
// The types an element may hold: scalars and vectors of 32 bit numbers, and
// structs of them.
// ****************************************************************************

struct Vector
{
    char scalar = 'f'; //!< 'f' float, 'i' int, 'u' uint.
    int components = 0;
};

//! \brief The vector named \c p_type, components 0 when it is not one.
Vector vectorOf(std::string_view p_type)
{
    static const std::map<std::string_view, Vector> known = {
        { "float", { 'f', 1 } }, { "vec2", { 'f', 2 } },  { "vec3", { 'f', 3 } },  { "vec4", { 'f', 4 } },
        { "int", { 'i', 1 } },   { "ivec2", { 'i', 2 } }, { "ivec3", { 'i', 3 } }, { "ivec4", { 'i', 4 } },
        { "uint", { 'u', 1 } },  { "uvec2", { 'u', 2 } }, { "uvec3", { 'u', 3 } }, { "uvec4", { 'u', 4 } },
    };
    auto found = known.find(p_type);
    return (found == known.end()) ? Vector{} : found->second;
}

//! \brief Alignment of a vector in std430: a vec3 aligns on 16 bytes.
std::uint32_t alignOf(Vector p_vector)
{
    return (p_vector.components == 1) ? 4u : (p_vector.components == 2) ? 8u : 16u;
}

std::uint32_t roundUp(std::uint32_t p_value, std::uint32_t p_alignment)
{
    return ((p_value + p_alignment - 1u) / p_alignment) * p_alignment;
}

//! \brief One vector of an element: its path from the element (".position",
//! or "" for an element that is a vector), its type and its offset.
struct Leaf
{
    std::string path;
    std::string type;
    Vector vector;
    std::uint32_t offset = 0u;
};

//! \brief A struct of the source.
struct StructDef
{
    std::vector<std::pair<std::string, std::string>> fields; //!< type, name
    std::string unsupported; //!< Why it cannot be emulated, empty if it can.
};

std::map<std::string, StructDef, std::less<>> parseStructs(std::string const& p_code)
{
    std::map<std::string, StructDef, std::less<>> structs;
    constexpr std::string_view keyword = "struct";
    for (std::size_t at = p_code.find(keyword); at != std::string::npos; at = p_code.find(keyword, at + 1u))
    {
        if (((at > 0u) && isWordChar(p_code[at - 1u])) || isWordChar(p_code[at + keyword.size()]))
        {
            continue;
        }
        const std::size_t name_at = skipSpaces(p_code, at + keyword.size());
        const std::string name(wordAt(p_code, name_at));
        const std::size_t open = skipSpaces(p_code, name_at + name.size());
        if (name.empty() || (open >= p_code.size()) || (p_code[open] != '{'))
        {
            continue;
        }
        const std::size_t close = matchingBracket(p_code, open);
        if (close == std::string::npos)
        {
            continue;
        }
        StructDef def;
        const std::string_view body = std::string_view(p_code).substr(open + 1u, close - open - 1u);
        std::size_t begin = 0u;
        while (begin < body.size())
        {
            std::size_t end = body.find(';', begin);
            if (end == std::string_view::npos)
            {
                end = body.size();
            }
            const std::string_view declaration = trimmed(body.substr(begin, end - begin));
            begin = end + 1u;
            if (declaration.empty())
            {
                continue;
            }
            if (declaration.find('[') != std::string_view::npos)
            {
                def.unsupported = "a field is an array";
                continue;
            }
            // "highp vec2 a, b": the type, then the names.
            std::vector<std::string> words;
            std::string current;
            for (char c : declaration)
            {
                if (isWordChar(c))
                {
                    current += c;
                }
                else if (!current.empty())
                {
                    words.push_back(current);
                    current.clear();
                }
            }
            if (!current.empty())
            {
                words.push_back(current);
            }
            std::erase_if(words, [](std::string const& p_word)
                          { return (p_word == "highp") || (p_word == "mediump") || (p_word == "lowp"); });
            if (words.size() < 2u)
            {
                continue;
            }
            if (vectorOf(words[0]).components == 0)
            {
                def.unsupported = "the field '" + words[1] + "' is a " + words[0] +
                                  " (only float, int, uint and their vectors are emulated)";
            }
            for (std::size_t k = 1u; k < words.size(); ++k)
            {
                def.fields.emplace_back(words[0], words[k]);
            }
        }
        structs.emplace(name, std::move(def));
    }
    return structs;
}

//! \brief The vectors of an element of type \c p_type in std430, and its
//! stride. An error says why the type is not emulated.
Result<std::vector<Leaf>> leavesOf(std::string const& p_type,
                                   std::map<std::string, StructDef, std::less<>> const& p_structs,
                                   std::uint32_t& p_stride)
{
    const Vector vector = vectorOf(p_type);
    if (vector.components != 0)
    {
        p_stride = roundUp(4u * std::uint32_t(vector.components), alignOf(vector));
        return std::vector<Leaf>{ Leaf{ "", p_type, vector, 0u } };
    }
    auto found = p_structs.find(p_type);
    if (found == p_structs.end())
    {
        return failure("its elements are a " + p_type +
                       " (only float, int, uint, their vectors, and structs of them are emulated)");
    }
    if (!found->second.unsupported.empty())
    {
        return failure("in the struct " + p_type + ", " + found->second.unsupported);
    }
    std::vector<Leaf> leaves;
    std::uint32_t offset = 0u;
    std::uint32_t largest = 4u;
    for (auto const& [type, name] : found->second.fields)
    {
        const Vector field = vectorOf(type);
        offset = roundUp(offset, alignOf(field));
        leaves.push_back(Leaf{ "." + name, type, field, offset });
        offset += 4u * std::uint32_t(field.components);
        largest = std::max(largest, alignOf(field));
    }
    p_stride = roundUp(offset, largest);
    return leaves;
}

//! \brief The GLSL that reads one vector from the words of the copy.
std::string fetchOf(Leaf const& p_leaf, std::string const& p_sampler)
{
    std::string parts;
    for (int k = 0; k < p_leaf.vector.components; ++k)
    {
        const std::string word =
            "compages_word(" + p_sampler + ", b + " + std::to_string(p_leaf.offset / 4u + std::uint32_t(k)) + ")";
        parts += (k == 0) ? "" : ", ";
        parts += (p_leaf.vector.scalar == 'f') ? "uintBitsToFloat(" + word + ")"
                 : (p_leaf.vector.scalar == 'i') ? "int(" + word + ")"
                                                 : word;
    }
    return (p_leaf.vector.components == 1) ? parts : p_leaf.type + "(" + parts + ")";
}

// ****************************************************************************
// The storage blocks of the source.
// ****************************************************************************

struct BlockSource
{
    EmulatedBlock block;
    std::string type;               //!< The type of an element.
    std::size_t begin = 0u;         //!< Where the declaration starts...
    std::size_t end = 0u;           //!< ...and ends (after its ';').
    std::vector<Leaf> leaves;
};

bool isBlockQualifier(std::string_view p_word)
{
    return (p_word == "readonly") || (p_word == "writeonly") || (p_word == "restrict") || (p_word == "coherent") ||
           (p_word == "volatile");
}

//! \brief The `binding = N` among the items of a layout, 0 if none.
int bindingIn(std::string_view p_items)
{
    std::size_t at = p_items.find("binding");
    while (at != std::string_view::npos)
    {
        if (((at == 0u) || !isWordChar(p_items[at - 1u])) && !isWordChar(p_items[at + 7u]))
        {
            std::size_t i = skipSpaces(p_items, at + 7u);
            if ((i < p_items.size()) && (p_items[i] == '='))
            {
                i = skipSpaces(p_items, i + 1u);
                return std::atoi(std::string(p_items.substr(i)).c_str());
            }
        }
        at = p_items.find("binding", at + 1u);
    }
    return 0;
}

//! \brief The word ending just before \c p_at (spaces skipped), and where
//! it starts.
std::string_view wordBefore(std::string const& p_code, std::size_t p_at, std::size_t& p_start)
{
    std::size_t end = p_at;
    while ((end > 0u) && isSpace(p_code[end - 1u]))
    {
        --end;
    }
    std::size_t start = end;
    while ((start > 0u) && isWordChar(p_code[start - 1u]))
    {
        --start;
    }
    p_start = start;
    return std::string_view(p_code).substr(start, end - start);
}

Result<std::vector<BlockSource>> parseBlocks(std::string const& p_code,
                                             std::map<std::string, StructDef, std::less<>> const& p_structs)
{
    std::vector<BlockSource> blocks;
    constexpr std::string_view keyword = "buffer";
    for (std::size_t at = p_code.find(keyword); at != std::string::npos; at = p_code.find(keyword, at + 1u))
    {
        if (((at > 0u) && isWordChar(p_code[at - 1u])) || isWordChar(p_code[at + keyword.size()]))
        {
            continue;
        }
        BlockSource source;
        const std::size_t name_at = skipSpaces(p_code, at + keyword.size());
        source.block.block = std::string(wordAt(p_code, name_at));
        const std::size_t open = skipSpaces(p_code, name_at + source.block.block.size());
        if (source.block.block.empty() || (open >= p_code.size()) || (p_code[open] != '{'))
        {
            continue;
        }
        const std::size_t close = matchingBracket(p_code, open);
        if (close == std::string::npos)
        {
            return failure("the storage block " + source.block.block + " is not closed");
        }
        std::size_t after = skipSpaces(p_code, close + 1u);
        if ((after < p_code.size()) && isWordChar(p_code[after]))
        {
            return failure("the storage block " + source.block.block +
                           " has an instance name: write its array directly (`buffer " + source.block.block +
                           " { T name[]; };`)");
        }
        if ((after >= p_code.size()) || (p_code[after] != ';'))
        {
            return failure("the storage block " + source.block.block + " does not end with ';'");
        }
        source.end = after + 1u;

        // Back over the qualifiers and the layout.
        std::size_t begin = at;
        while (true)
        {
            std::size_t start = 0u;
            const std::string_view word = wordBefore(p_code, begin, start);
            if (isBlockQualifier(word))
            {
                begin = start;
                continue;
            }
            std::size_t end = begin;
            while ((end > 0u) && isSpace(p_code[end - 1u]))
            {
                --end;
            }
            if ((end > 0u) && (p_code[end - 1u] == ')'))
            {
                int depth = 0;
                std::size_t i = end - 1u;
                for (;; --i)
                {
                    depth += (p_code[i] == ')') ? 1 : (p_code[i] == '(') ? -1 : 0;
                    if ((depth == 0) || (i == 0u))
                    {
                        break;
                    }
                }
                std::size_t layout_start = 0u;
                if (wordBefore(p_code, i, layout_start) == "layout")
                {
                    source.block.binding = bindingIn(std::string_view(p_code).substr(i + 1u, end - i - 2u));
                    begin = layout_start;
                    continue;
                }
            }
            break;
        }
        source.begin = begin;

        // One member: `T name[];`.
        const std::string_view body = trimmed(std::string_view(p_code).substr(open + 1u, close - open - 1u));
        const std::size_t semicolon = body.find(';');
        const std::string_view member = trimmed(body.substr(0u, semicolon));
        if ((semicolon == std::string_view::npos) || !trimmed(body.substr(semicolon + 1u)).empty() ||
            !member.ends_with("]") || (member.find('[') == std::string_view::npos) ||
            !trimmed(member.substr(member.find('[') + 1u, member.size() - member.find('[') - 2u)).empty())
        {
            return failure("the storage block " + source.block.block +
                           " must hold exactly one array of unknown size (`T name[];`) to be emulated");
        }
        const std::string_view declaration = trimmed(member.substr(0u, member.find('[')));
        const std::size_t space = declaration.find_last_of(" \t\n");
        if (space == std::string_view::npos)
        {
            return failure("the storage block " + source.block.block + ": cannot read its member");
        }
        source.block.array = std::string(trimmed(declaration.substr(space + 1u)));
        std::string_view type = trimmed(declaration.substr(0u, space));
        if (type.find_last_of(" \t\n") != std::string_view::npos)
        {
            type = trimmed(type.substr(type.find_last_of(" \t\n") + 1u)); // "highp vec2"
        }
        source.type = std::string(type);
        auto leaves = leavesOf(source.type, p_structs, source.block.stride);
        if (!leaves)
        {
            return failure("the storage block " + source.block.block + " cannot be emulated: " + leaves.error());
        }
        source.leaves = leaves.take();
        for (Leaf const& leaf : source.leaves)
        {
            source.block.fields.push_back(
                EmulatedBlock::Field{ source.block.array + leaf.path, leaf.type, leaf.offset });
        }
        blocks.push_back(std::move(source));
    }
    return blocks;
}

// ****************************************************************************
// What has no equivalent, and the built-in variables of a compute shader.
// ****************************************************************************

//! \brief Why a word cannot be emulated, empty if it can.
std::string refusal(std::string_view p_word)
{
    static const std::set<std::string_view, std::less<>> atomics = {
        "atomicAdd",     "atomicMin",          "atomicMax",           "atomicAnd",         "atomicOr",
        "atomicXor",     "atomicExchange",     "atomicCompSwap",      "atomicCounter",     "atomicCounterIncrement",
        "atomicCounterDecrement", "atomic_uint",
    };
    static const std::set<std::string_view, std::less<>> images = {
        "imageLoad",  "imageStore",   "imageSize",    "imageAtomicAdd", "imageAtomicExchange", "image1D",
        "image2D",    "image3D",      "imageCube",    "image2DArray",   "imageBuffer",         "iimage2D",
        "iimage3D",   "uimage2D",     "uimage3D",     "iimageBuffer",   "uimageBuffer",
    };
    if ((p_word == "shared") || (p_word == "barrier") || p_word.starts_with("memoryBarrier") ||
        (p_word == "groupMemoryBarrier"))
    {
        return "uses '" + std::string(p_word) +
               "': memory shared by a work group does not exist without compute shaders. To read every "
               "element from every invocation (an N-body), write the loop with COMPAGES_FOR_EACH of "
               "<compages/tiles.glsl>: shared on a device with compute, a direct loop elsewhere";
    }
    if (atomics.contains(p_word))
    {
        return "uses '" + std::string(p_word) + "': atomic operations do not exist without compute shaders";
    }
    if (images.contains(p_word))
    {
        return "uses '" + std::string(p_word) +
               "': images (load and store) do not exist without compute shaders; a fragment shader drawing "
               "into a Framebuffer (TexturePingPong) does the same on every device";
    }
    return {};
}

std::string_view builtin(std::string_view p_word)
{
    static const std::map<std::string_view, std::string_view, std::less<>> renamed = {
        { "gl_GlobalInvocationID", "compages_GlobalInvocationID" },
        { "gl_LocalInvocationID", "compages_LocalInvocationID" },
        { "gl_WorkGroupID", "compages_WorkGroupID" },
        { "gl_LocalInvocationIndex", "compages_LocalInvocationIndex" },
        { "gl_NumWorkGroups", "compages_NumWorkGroups" },
        { "gl_WorkGroupSize", "compages_WorkGroupSize" },
    };
    auto found = renamed.find(p_word);
    return (found == renamed.end()) ? std::string_view{} : found->second;
}

//! \brief Does an assignment (or ++, --) start at \c p_at?
bool assignmentAt(std::string const& p_code, std::size_t p_at)
{
    if (p_at + 1u >= p_code.size())
    {
        return false;
    }
    const char c = p_code[p_at];
    const char next = p_code[p_at + 1u];
    if (c == '=')
    {
        return next != '=';
    }
    if (((c == '+') && (next == '+')) || ((c == '-') && (next == '-')))
    {
        return true;
    }
    if ((std::string_view("+-*/%&|^").find(c) != std::string_view::npos) && (next == '='))
    {
        return true;
    }
    return (((c == '<') && (next == '<')) || ((c == '>') && (next == '>'))) && (p_at + 2u < p_code.size()) &&
           (p_code[p_at + 2u] == '=');
}

//! \brief The local size of `layout(local_size_x = ...) in;`, and the range
//! of that statement.
bool parseLocalSize(std::string const& p_code, std::array<int, 3>& p_size, std::size_t& p_begin, std::size_t& p_end)
{
    for (std::size_t at = p_code.find("layout"); at != std::string::npos; at = p_code.find("layout", at + 1u))
    {
        const std::size_t open = skipSpaces(p_code, at + 6u);
        if ((open >= p_code.size()) || (p_code[open] != '('))
        {
            continue;
        }
        const std::size_t close = matchingBracket(p_code, open);
        const std::string_view items = std::string_view(p_code).substr(open + 1u, close - open - 1u);
        if (items.find("local_size_") == std::string_view::npos)
        {
            continue;
        }
        for (int axis = 0; axis < 3; ++axis)
        {
            const std::string key = std::string("local_size_") + "xyz"[axis];
            const std::size_t k = items.find(key);
            if (k != std::string_view::npos)
            {
                std::size_t i = skipSpaces(items, k + key.size());
                i = skipSpaces(items, i + 1u); // the '='
                p_size[std::size_t(axis)] = std::atoi(std::string(items.substr(i)).c_str());
            }
        }
        p_begin = at;
        p_end = p_code.find(';', close);
        p_end = (p_end == std::string::npos) ? close + 1u : p_end + 1u;
        return true;
    }
    return false;
}

} // namespace

namespace
{

//! \brief The rewriting of emulateCompute() (\c p_drawing false) and of
//! emulateStorageReads() (true: reads only, main kept, nothing captured).
Result<EmulatedCompute> emulate(std::string_view p_source, bool p_drawing, std::vector<std::string> const& p_defined)
{
    const std::string kind = p_drawing ? "shader" : "compute shader";

    // What the program holds, once #if and #ifdef have chosen: one source may
    // declare a block for one value of a macro and another for another.
    std::string error;
    const std::string active = keepActiveLines(p_source, p_defined, error);
    if (!error.empty())
    {
        return failure("the " + kind + " cannot be emulated: " + error);
    }
    const std::string_view shader = active;
    EmulatedCompute out;
    const std::string with_directives = blankComments(shader);
    const std::string code = blankDirectives(with_directives);
    std::vector<Edit> edits;

    // The version (and extensions): what is added goes after them.
    std::size_t header_at = std::string::npos;
    for (std::size_t begin = 0u; begin < with_directives.size();)
    {
        std::size_t end = with_directives.find('\n', begin);
        end = (end == std::string::npos) ? with_directives.size() : end;
        const std::string_view line = trimmed(std::string_view(with_directives).substr(begin, end - begin));
        if (line.starts_with("#"))
        {
            const std::string_view directive = trimmed(line.substr(1u));
            if (directive.starts_with("version") || (directive.starts_with("extension") && (header_at != std::string::npos)))
            {
                header_at = std::min(end + 1u, with_directives.size());
            }
        }
        begin = end + 1u;
    }
    if (header_at == std::string::npos)
    {
        return failure("the " + kind + " has no #version");
    }

    std::size_t local_begin = 0u;
    std::size_t local_end = 0u;
    if (!p_drawing)
    {
        if (!parseLocalSize(code, out.local_size, local_begin, local_end))
        {
            return failure("the compute shader declares no layout(local_size_x = ...) in;");
        }
        edits.push_back(Edit{ local_begin, local_end - local_begin, "" });
    }

    const auto structs = parseStructs(code);
    std::vector<BlockSource> blocks;
    COMPAGES_TRY_ASSIGN(blocks, parseBlocks(code, structs));
    std::map<std::string, std::size_t, std::less<>> by_array;
    for (std::size_t i = 0u; i < blocks.size(); ++i)
    {
        by_array.emplace(blocks[i].block.array, i);
    }
    const auto inDeclaration = [&](std::size_t p_at)
    {
        if ((p_at >= local_begin) && (p_at < local_end))
        {
            return true;
        }
        return std::any_of(blocks.begin(),
                           blocks.end(),
                           [p_at](BlockSource const& p_block) { return (p_at >= p_block.begin) && (p_at < p_block.end); });
    };

    // Every word: the accesses to the blocks, the built-in variables, main,
    // and what is refused.
    bool has_main = false;
    std::map<std::string, std::string, std::less<>> written_index; // array -> the index it is written at
    std::size_t at = 0u;
    while (at < code.size())
    {
        if (!isWordChar(code[at]) || ((at > 0u) && (isWordChar(code[at - 1u]) || (code[at - 1u] == '.'))) ||
            (std::isdigit(static_cast<unsigned char>(code[at])) != 0) || inDeclaration(at))
        {
            ++at;
            continue;
        }
        const std::string_view word = wordAt(code, at);
        const std::size_t after_word = at + word.size();

        if (std::string why = refusal(word); !why.empty())
        {
            return failure("this " + kind + " cannot run on a device without compute shaders: it " + why);
        }
        if (p_drawing && (word == "main"))
        {
            at = after_word;
            continue;
        }
        if (auto renamed = builtin(word); !renamed.empty())
        {
            edits.push_back(Edit{ at, word.size(), std::string(renamed) });
            at = after_word;
            continue;
        }
        if (word == "main")
        {
            std::size_t start = 0u;
            const std::size_t open = skipSpaces(code, after_word);
            if ((wordBefore(code, at, start) == "void") && (open < code.size()) && (code[open] == '('))
            {
                edits.push_back(Edit{ at, word.size(), "compages_main" });
                has_main = true;
            }
            at = after_word;
            continue;
        }
        auto block = by_array.find(word);
        if (block == by_array.end())
        {
            at = after_word;
            continue;
        }

        BlockSource& source = blocks[block->second];
        std::string const& array = source.block.array;
        const std::size_t open = skipSpaces(code, after_word);
        if ((open < code.size()) && (code[open] == '.'))
        {
            // array.length()
            const std::size_t method = skipSpaces(code, open + 1u);
            const std::size_t paren = skipSpaces(code, method + 6u);
            if ((wordAt(code, method) == "length") && (paren < code.size()) && (code[paren] == '('))
            {
                const std::size_t close = matchingBracket(code, paren);
                edits.push_back(Edit{ at, close + 1u - at, "compages_count_" + array });
                at = close + 1u;
                continue;
            }
        }
        if ((open >= code.size()) || (code[open] != '['))
        {
            return failure("this " + kind + " uses the storage array '" + array +
                           "' as a whole: only its elements (" + array + "[i]) and " + array +
                           ".length() can be emulated");
        }
        const std::size_t close = matchingBracket(code, open);
        if (close == std::string::npos)
        {
            return failure("an access to '" + array + "' is not closed");
        }

        // What follows the element: a path (.position, [k], .x), then maybe
        // an assignment. A ++ or -- in front is a write too.
        std::size_t next = skipSpaces(code, close + 1u);
        while (next < code.size())
        {
            if (code[next] == '.')
            {
                next = skipSpaces(code, next + 1u);
                next = skipSpaces(code, next + wordAt(code, next).size());
            }
            else if (code[next] == '[')
            {
                next = skipSpaces(code, matchingBracket(code, next) + 1u);
            }
            else
            {
                break;
            }
        }
        std::size_t before = at;
        while ((before > 0u) && isSpace(code[before - 1u]))
        {
            --before;
        }
        const bool prefix = (before >= 2u) && (((code[before - 1u] == '+') && (code[before - 2u] == '+')) ||
                                               ((code[before - 1u] == '-') && (code[before - 2u] == '-')));
        if ((prefix || assignmentAt(code, next)) && p_drawing)
        {
            return failure("this shader writes the storage block '" + source.block.block +
                           "': a vertex or fragment shader writes a storage block only with OpenGL 4.3 or OpenGL ES "
                           "3.1. Here it can read one");
        }
        if (prefix || assignmentAt(code, next))
        {
            // A write goes to the record when its index is the own one, and
            // to a slot thrown away otherwise (checked in the shader). Several
            // different indices for one block are refused when loading: that
            // invocation means to write several elements, which would be lost.
            std::string index;
            for (char c : std::string_view(code).substr(open + 1u, close - open - 1u))
            {
                if (!isSpace(c))
                {
                    index += c;
                }
            }
            auto first = written_index.emplace(array, index).first;
            if (first->second != index)
            {
                return failure("this compute shader writes '" + array + "' at several indices (" + first->second +
                               " and " + index +
                               "). Without compute shaders, an invocation writes only one element of a block, its "
                               "own (gl_GlobalInvocationID.x): split the work so that each invocation writes one "
                               "element");
            }
            edits.push_back(Edit{ at, open + 1u - at, "compages_out_" + array + "[compages_slot(int(" });
            edits.push_back(Edit{ close, 1u, "))]" });
            source.block.written = true;
            at = open + 1u; // the index may read blocks too
            continue;
        }
        edits.push_back(Edit{ at, open + 1u - at, "compages_read_" + array + "(int(" });
        edits.push_back(Edit{ close, 1u, "))" });
        at = open + 1u; // the index may read blocks too
    }
    if (!has_main && !p_drawing)
    {
        return failure("the compute shader has no void main()");
    }

    // Written blocks: one interleaved record, or one buffer each.
    std::vector<BlockSource*> written;
    for (BlockSource& source : blocks)
    {
        if (source.block.written)
        {
            written.push_back(&source);
        }
    }
    if (written.empty() && !p_drawing)
    {
        return failure("this compute shader writes no storage block: there is nothing to capture");
    }
    out.separate = written.size() > 1u;
    if (written.size() > 4u)
    {
        return failure("this compute shader writes " + std::to_string(written.size()) +
                       " storage blocks; without compute shaders, at most 4 can be written by one dispatch");
    }

    // The code of each block, where it was declared.
    std::string outputs;
    std::string captures;
    std::string inits;
    for (BlockSource& source : blocks)
    {
        EmulatedBlock& block = source.block;
        std::string const& a = block.array;
        std::string const& t = source.type;
        const std::string sampler = "compages_data_" + a;
        // The copy holds the whole buffer: compages_base_ is where the bound
        // range starts, in words.
        std::string text = "\nuniform highp usampler2D " + sampler + ";\nuniform int compages_count_" + a +
                           ";\nuniform int compages_base_" + a + ";\n";
        text += t + " compages_fetch_" + a + "(int j)\n{\n    int b = compages_base_" + a + " + j * " +
                std::to_string(block.stride / 4u) + ";\n";
        if (source.leaves.size() == 1u && source.leaves[0].path.empty())
        {
            text += "    return " + fetchOf(source.leaves[0], sampler) + ";\n}\n";
        }
        else
        {
            text += "    " + t + " v;\n";
            for (Leaf const& leaf : source.leaves)
            {
                text += "    v" + leaf.path + " = " + fetchOf(leaf, sampler) + ";\n";
            }
            text += "    return v;\n}\n";
        }
        if (block.written)
        {
            // [0] the own element, [1] where writes to another one go.
            text += t + " compages_out_" + a + "[2];\n";
            text += t + " compages_read_" + a + "(int j)\n{\n    if (j == compages_index)\n    {\n        return compages_out_" +
                    a + "[0];\n    }\n    return compages_fetch_" + a + "(j);\n}\n";
            inits += "    compages_out_" + a + "[0] = compages_fetch_" + a + "(compages_index);\n";

            // The record, in the order of the memory, padding included.
            std::uint32_t offset = 0u;
            int pad = 0;
            const auto padTo = [&](std::uint32_t p_offset)
            {
                for (; offset < p_offset; offset += 4u)
                {
                    const std::string name = "compages_pad_" + a + "_" + std::to_string(pad++);
                    outputs += "out float " + name + ";\n";
                    captures += "    " + name + " = 0.0;\n";
                    block.outputs.push_back(name);
                }
            };
            for (std::size_t k = 0u; k < source.leaves.size(); ++k)
            {
                Leaf const& leaf = source.leaves[k];
                padTo(leaf.offset);
                const std::string name = "compages_tf_" + a + "_" + std::to_string(k);
                outputs += std::string((leaf.vector.scalar == 'f') ? "" : "flat ") + "out " + leaf.type + " " + name + ";\n";
                captures += "    " + name + " = compages_out_" + a + "[0]" + leaf.path + ";\n";
                block.outputs.push_back(name);
                offset += 4u * std::uint32_t(leaf.vector.components);
            }
            padTo(block.stride);
            if (out.separate && (block.outputs.size() != 1u))
            {
                return failure("this compute shader writes several storage blocks, and '" + block.block +
                               "' holds structs or padded vectors: without compute shaders, several written blocks "
                               "must each hold float, int, uint, vec2, vec4 (or ivec, uvec) elements");
            }
        }
        else
        {
            text += t + " compages_read_" + a + "(int j)\n{\n    return compages_fetch_" + a + "(j);\n}\n";
        }
        text += "#line " + std::to_string(lineNumberAt(with_directives, source.end)) + "\n";
        edits.push_back(Edit{ source.begin, source.end - source.begin, std::move(text) });
        out.blocks.push_back(block);
    }

    // A drawing shader: how to read a word of a copy, nothing else.
    const std::string word = "uint compages_word(highp usampler2D s, int n)\n{\n"
                             "    return texelFetch(s, ivec2(n % compages_width, n / compages_width), 0).r;\n}\n";
    if (p_drawing)
    {
        std::string header = "uniform int compages_width;\n" + word;
        header += "#line " + std::to_string(lineNumberAt(with_directives, header_at)) + "\n";
        edits.push_back(Edit{ header_at, 0u, std::move(header) });
        out.text = applyEdits(shader, std::move(edits));
        return out;
    }

    // The header, after the version: the built-in variables and how to read a
    // word of a copy.
    const std::array<int, 3>& size = out.local_size;
    std::string header = "#define COMPAGES_EMULATED_COMPUTE 1\n";
    header += "uniform uvec3 compages_groups;\nuniform int compages_width;\n";
    header += "const uvec3 compages_WorkGroupSize = uvec3(" + std::to_string(size[0]) + "u, " + std::to_string(size[1]) +
              "u, " + std::to_string(size[2]) + "u);\n";
    header += "uvec3 compages_GlobalInvocationID;\nuvec3 compages_LocalInvocationID;\nuvec3 compages_WorkGroupID;\n"
              "uint compages_LocalInvocationIndex;\nuvec3 compages_NumWorkGroups;\nint compages_index;\n";
    header += "int compages_slot(int j)\n{\n    return (j == compages_index) ? 0 : 1;\n}\n";
    header += word;
    header += "#line " + std::to_string(lineNumberAt(with_directives, header_at)) + "\n";
    edits.push_back(Edit{ header_at, 0u, std::move(header) });

    // The main that runs one invocation and writes its record.
    std::string footer = "\n" + outputs + "void main()\n{\n";
    footer += "    compages_index = gl_VertexID;\n"
              "    uint l = uint(gl_VertexID);\n"
              "    uvec3 compages_all = compages_groups * compages_WorkGroupSize;\n"
              "    compages_GlobalInvocationID = uvec3(l % compages_all.x, (l / compages_all.x) % compages_all.y, l / (compages_all.x * compages_all.y));\n"
              "    compages_LocalInvocationID = compages_GlobalInvocationID % compages_WorkGroupSize;\n"
              "    compages_WorkGroupID = compages_GlobalInvocationID / compages_WorkGroupSize;\n"
              "    compages_LocalInvocationIndex = compages_LocalInvocationID.x + compages_WorkGroupSize.x * "
              "(compages_LocalInvocationID.y + compages_WorkGroupSize.y * compages_LocalInvocationID.z);\n"
              "    compages_NumWorkGroups = compages_groups;\n";
    footer += inits;
    footer += "    compages_main();\n";
    footer += captures;
    footer += "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n}\n";
    edits.push_back(Edit{ shader.size(), 0u, std::move(footer) });

    out.text = applyEdits(shader, std::move(edits));
    return out;
}

} // namespace

Result<EmulatedCompute> emulateCompute(std::string_view p_source)
{
    return emulate(p_source, false, { "COMPAGES_EMULATED_COMPUTE" });
}

Result<EmulatedCompute> emulateStorageReads(std::string_view p_source, std::vector<std::string> const& p_defined)
{
    return emulate(p_source, true, p_defined);
}

bool declaresStorageBlock(std::string_view p_source)
{
    const std::string code = blankDirectives(blankComments(p_source));
    for (std::size_t at = code.find("buffer"); at != std::string::npos; at = code.find("buffer", at + 1u))
    {
        const bool alone = ((at == 0u) || !isWordChar(code[at - 1u])) &&
                           ((at + 6u >= code.size()) || !isWordChar(code[at + 6u]));
        if (alone)
        {
            return true;
        }
    }
    return false;
}

} // namespace compages::gpu::detail

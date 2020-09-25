// Copyright (C) 2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef LEXY_DSL_CONDITION_HPP_INCLUDED
#define LEXY_DSL_CONDITION_HPP_INCLUDED

#include <lexy/dsl/base.hpp>

namespace lexy
{
template <bool Expected, typename Reader>
struct _expected_pattern_error
{
public:
    constexpr explicit _expected_pattern_error(typename Reader::iterator pos) noexcept : _pos(pos)
    {}

    constexpr auto position() const noexcept
    {
        return _pos;
    }

private:
    typename Reader::iterator _pos;
};

struct expected_pattern
{
    template <typename Reader>
    using error = _expected_pattern_error<true, Reader>;
};

struct unexpected_pattern
{
    template <typename Reader>
    using error = _expected_pattern_error<false, Reader>;
};
} // namespace lexy

namespace lexyd
{
template <typename Pattern, bool Expected>
struct _if : rule_base
{
    static constexpr auto has_matcher = true;

    struct matcher
    {
        template <typename Reader>
        LEXY_DSL_FUNC bool match(Reader& reader)
        {
            auto copy = reader;
            return Pattern::matcher::match(copy) == Expected;
        }
    };

    template <typename NextParser>
    struct parser
    {
        template <typename Context, typename Reader, typename... Args>
        LEXY_DSL_FUNC auto parse(Context& context, Reader& reader, Args&&... args) ->
            typename Context::result_type
        {
            if (matcher::match(reader))
                return NextParser::parse(context, reader, LEXY_FWD(args)...);
            else
                return LEXY_MOV(context).error(reader,
                                               lexy::_expected_pattern_error<Expected, Reader>(
                                                   reader.cur()));
        }
    };
};

/// Check if at this reader position, Pattern would match, but don't actually consume any characters
/// if it does.
template <typename Pattern>
LEXY_CONSTEVAL auto if_(Pattern)
{
    static_assert(lexy::is_pattern<Pattern>);
    return _if<Pattern, true>{};
}

/// Check if at this reader position, Pattern would not match, but don't actually consume any
/// characters if it does.
template <typename Pattern>
LEXY_CONSTEVAL auto unless(Pattern)
{
    static_assert(lexy::is_pattern<Pattern>);
    return _if<Pattern, false>{};
}
} // namespace lexyd

namespace lexyd
{
template <typename Pattern>
struct _not : rule_base
{
    static constexpr auto has_matcher = true;

    struct matcher
    {
        template <typename Reader>
        LEXY_DSL_FUNC bool match(Reader& reader)
        {
            // We match if we didn't match the pattern.
            // In other words:
            // If we return true, we haven't matched Pattern and haven't consumed any reader.
            // If we return false, we have matched Pattern and have consumed it.
            return !Pattern::matcher::match(reader);
        }
    };

    // As parser, we use the one from unless().
    // Otherwise, the generated error is at the wrong position.
    template <typename NextParser>
    using parser = typename _if<Pattern, false>::template parser<NextParser>;
};

/// Check that Pattern doesn't match.
/// This is used for something like `opt(!pattern >> rule)`, which is equivalent to `pattern |
/// rule`.
template <typename Pattern>
LEXY_CONSTEVAL auto operator!(Pattern)
{
    static_assert(lexy::is_pattern<Pattern>);
    return _not<Pattern>{};
}
} // namespace lexyd

#endif // LEXY_DSL_CONDITION_HPP_INCLUDED


#pragma once
#include <array>
#include <concepts>
#include <utility>
#include <cstddef>

namespace Crawler
{

template <typename Y, std::integral T, T... ints> requires requires(T t) { static_cast<Y>(t); }
constexpr auto BuildArrayFromIntegerSequence(std::integer_sequence<T, ints...>) {
    return std::array<Y, sizeof...(ints)>{{ static_cast<Y>(ints)... }};
}

template <auto A, typename B>
struct Range;

template <typename T, T Lower, T... Base>
struct Range<Lower, std::integer_sequence<T, Base...>> {
    using type = std::integer_sequence<T, (Lower + Base)...>;
};

template <typename T, T Lower, T Upper>
struct InRange {
    using type = Range<Lower, std::make_integer_sequence<T, Upper - Lower + 1>>::type;
};

template <typename A, typename B>
struct Concat;

template <std::integral T, T... A, T... B>
struct Concat<std::integer_sequence<T, A...>, std::integer_sequence<T, B...>> {
    using type = std::integer_sequence<T, A..., B...>;
};

template<typename S>
struct Contains {
    template<typename T, auto Value>
    struct Apply;
};

template<typename T, T... Values>
struct Contains<std::integer_sequence<T, Values...>> {
    template<typename U, auto Value>
    struct Apply {
        static constexpr bool val = !((Value == Values) || ...);
    };
};

template <typename Sequence, typename Accumulator, typename Predicate>
struct Filter;

template <typename T, T... Accumulator, typename Predicate>
struct Filter<std::integer_sequence<T>, std::integer_sequence<T, Accumulator...>, Predicate> {
    using type = std::integer_sequence<T, Accumulator...>;
};

template <typename T, T Head, T... Tail, T... Accumulator, typename Predicate>
struct Filter<std::integer_sequence<T, Head, Tail...>, std::integer_sequence<T, Accumulator...>, Predicate> {
private:
    using NextAccumulator = std::conditional_t<Predicate::template Apply<T, Head>::val, std::integer_sequence<T, Accumulator..., Head>, std::integer_sequence<T, Accumulator...>>;

public:
    using type = typename Filter<std::integer_sequence<T, Tail...>, NextAccumulator, Predicate>::type;
};

using CRAWLER_LEADING_SURROGATES_RANGE_TYPE = InRange<uint16_t, 0xD800, 0xDBFF>::type;
using CRAWLER_TRAILING_SURROGATES_RANGE_TYPE = InRange<uint16_t, 0xDC00, 0xDFFF>::type;
using CRAWLER_NON_CHARACTER_RANGE_TYPE = InRange<uint16_t, 0xFDD0, 0xFDEF>::type;
using CRAWLER_NON_CHARACTER_TYPE = std::integer_sequence<uint16_t,
    0xFFFE, 0xFFFF, 0x1FFFE, 0x1FFFF, 0x2FFFE, 0x2FFFF, 0x3FFFE, 0x3FFFF, 0x4FFFE, 0x4FFFF, 0x5FFFE,
    0x5FFFF, 0x6FFFE, 0x6FFFF, 0x7FFFE, 0x7FFFF, 0x8FFFE, 0x8FFFF, 0x9FFFE, 0x9FFFF, 0xAFFFE, 0xAFFFF,
    0xBFFFE, 0xBFFFF, 0xCFFFE, 0xCFFFF, 0xDFFFE, 0xDFFFF, 0xEFFFE, 0xEFFFF, 0xFFFFE, 0xFFFFF, 0x10FFFE, 0x10FFFF>;
using CRAWLER_CONTROL_RANGE_TYPE = InRange<uint16_t, 0x007F, 0x009F>::type;
using CRAWLER_NULL_BYTE = std::integer_sequence<uint16_t, 0x0000>;
using CRAWLER_WHITESPACES_TYPE = std::integer_sequence<uint16_t, 0x0009, 0x000A, 0x000C, 0x000D, 0x0020>;
using CRAWLER_WHITESPACES_AND_NULL_TYPE = Concat<CRAWLER_NULL_BYTE, CRAWLER_WHITESPACES_TYPE>::type;
using CRAWLER_CR_TYPE = std::integer_sequence<uint16_t, 0x000D>;
using CRAWLER_LF_TYPE = std::integer_sequence<uint16_t, 0x000A>;
using CRAWLER_UPPER_CASE_LETTERS_TYPE = InRange<uint8_t, 0x41, 0x5A>::type;
using CRAWLER_LOWER_CASE_LETTERS_TYPE = InRange<uint8_t, 0x61, 0x7A>::type;
using CRAWLER_LETTERS_TYPE = Concat<CRAWLER_UPPER_CASE_LETTERS_TYPE, CRAWLER_LOWER_CASE_LETTERS_TYPE>::type;
using CRAWLER_SPACE_CONTROL_TYPE = InRange<uint8_t, 0x00, 0x20>::type;
using CRAWLER_CONTROL_TYPE = Filter<CRAWLER_CONTROL_RANGE_TYPE, std::integer_sequence<uint16_t>, Contains<CRAWLER_WHITESPACES_AND_NULL_TYPE>>::type;

/* Spaces single byte. */
using CRAWLER_ONE_BYTE_SPACE_TYPE = std::integer_sequence<uint8_t, 0x09, 0x0A, 0x0C, 0x0D, 0x20>;

/* Open meta tag. */
using CRAWLER_M_TYPE = std::integer_sequence<uint8_t, 0x4D, 0x6D>;
using CRAWLER_E_TYPE = std::integer_sequence<uint8_t, 0x45, 0x65>;
using CRAWLER_T_TYPE = std::integer_sequence<uint8_t, 0x54, 0x74>;
using CRAWLER_A_TYPE = std::integer_sequence<uint8_t, 0x41, 0x61>;
using CRAWLER_S_TYPE = Concat<CRAWLER_ONE_BYTE_SPACE_TYPE, std::integer_sequence<uint8_t, 0x2F>>::type;

/* ESQ */
using CRAWLER_ESQ_TYPE = std::integer_sequence<uint8_t, 0x21, 0x2F, 0x3F>;

/* Skip sequence. */
using SKIP_SEQUENCE_TYPE = Concat<CRAWLER_ONE_BYTE_SPACE_TYPE, std::integer_sequence<uint8_t, 0x3E>>::type;

/* Abort bytes. */
using ABORT_BYTES_TYPE = std::integer_sequence<uint8_t, 0x4D, 0x6D>;

/* Quotes. */
using QUOTES_TYPE = std::integer_sequence<uint8_t, 0x22, 0x27>;

/* Spaces and semicolon. */
using SPACE_AND_SEMICOLON_TYPE = Concat<CRAWLER_ONE_BYTE_SPACE_TYPE, std::integer_sequence<uint8_t, 0x3B>>::type;

/* Materialization. */
inline constexpr auto CRAWLER_SURROGATES = BuildArrayFromIntegerSequence<char32_t, uint16_t>(Concat<CRAWLER_LEADING_SURROGATES_RANGE_TYPE, CRAWLER_TRAILING_SURROGATES_RANGE_TYPE>::type{});
inline constexpr auto CRAWLER_NON_CHARACTER = BuildArrayFromIntegerSequence<char32_t, uint16_t>(Concat<CRAWLER_NON_CHARACTER_RANGE_TYPE, CRAWLER_NON_CHARACTER_TYPE>::type{});
inline constexpr auto CRAWLER_CONTROL = BuildArrayFromIntegerSequence<char32_t, uint16_t>(CRAWLER_CONTROL_TYPE{});
inline constexpr auto CRAWLER_CRLF = BuildArrayFromIntegerSequence<char32_t, uint16_t>(Concat<CRAWLER_CR_TYPE, CRAWLER_LF_TYPE>::type{});
inline constexpr auto CRAWLER_CR = BuildArrayFromIntegerSequence<char32_t, uint16_t>(CRAWLER_CR_TYPE{});
inline constexpr auto CRAWLER_UPPERCASE_LETTERS = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_UPPER_CASE_LETTERS_TYPE{});
inline constexpr auto CRAWLER_LETTERS = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_LETTERS_TYPE{});
inline constexpr auto CRAWLER_SPACE_CONTROL = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_SPACE_CONTROL_TYPE{});

/* Spaces single byte. */
inline constexpr auto CRAWLER_ONE_BYTE_SPACE = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_ONE_BYTE_SPACE_TYPE{});

/* Open meta tag. */
inline constexpr auto CRAWLER_M = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_M_TYPE{});
inline constexpr auto CRAWLER_E = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_E_TYPE{});
inline constexpr auto CRAWLER_T = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_T_TYPE{});
inline constexpr auto CRAWLER_A = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_A_TYPE{});
inline constexpr auto CRAWLER_S = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_S_TYPE{});

/* ESQ */
inline constexpr auto CRAWLER_ESQ = BuildArrayFromIntegerSequence<std::byte, uint8_t>(CRAWLER_ESQ_TYPE{});

/* Skip sequence. */
inline constexpr auto SKIP_SEQUENCE = BuildArrayFromIntegerSequence<std::byte, uint8_t>(SKIP_SEQUENCE_TYPE{});

/* Abort bytes. */
inline constexpr auto ABORT_BYTES = BuildArrayFromIntegerSequence<std::byte, uint8_t>(ABORT_BYTES_TYPE{});

/* Quotes. */
inline constexpr auto QUOTES = BuildArrayFromIntegerSequence<std::byte, uint8_t>(QUOTES_TYPE{});

/* Spaces and semicolon. */
inline constexpr auto SPACE_AND_SEMICOLON = BuildArrayFromIntegerSequence<std::byte, uint8_t>(SPACE_AND_SEMICOLON_TYPE{});

}
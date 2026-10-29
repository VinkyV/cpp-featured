/*
 * Copyright (c) 2026 VinkyV
 *
 * Portions of this file are derived from safe-cpp:
 * https://github.com/cppalliance/safe-cpp
 *
 * Original code copyright its respective copyright holders.
 * The original code is licensed under the Boost Software
 * License - Version 1.0 (BSL-1.0).
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#if defined(__has_builtin)
#  define STD2_HAS_BUILTIN(x) __has_builtin(x)
#else
#  define STD2_HAS_BUILTIN(x) 0
#endif

// Location builtins: GCC >= 4.8 (FILE/LINE), GCC >= 9, Clang >= 9, MSVC 19.26+.
#if defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1926)
#  define STD2_BUILTIN_FILE()     __builtin_FILE()
#  define STD2_BUILTIN_FUNCTION() __builtin_FUNCTION()
#  define STD2_BUILTIN_LINE()     __builtin_LINE()
#  if defined(_MSC_VER) || STD2_HAS_BUILTIN(__builtin_COLUMN)
#    define STD2_BUILTIN_COLUMN() __builtin_COLUMN()
#  else
#    define STD2_BUILTIN_COLUMN() 0
#  endif
#else
#  define STD2_BUILTIN_FILE()     "<unknown>"
#  define STD2_BUILTIN_FUNCTION() "<unknown>"
#  define STD2_BUILTIN_LINE()     0
#  define STD2_BUILTIN_COLUMN()   0
#endif

#if defined(_MSC_VER)
#  define STD2_NOINLINE __declspec(noinline)
#else
#  define STD2_NOINLINE __attribute__((noinline))
#endif

////////////////////////////////////////////////////////////////////////////////
// Panic categories (the original coordinates these with the Circle compiler;
// here they are informational and passed to the handler).
enum class panic_code {
  generic,
  bounds,
  divide_by_zero,
  lifetime,
  utf,
};

////////////////////////////////////////////////////////////////////////////////
// source_location

class source_location {
public:
  static source_location current(
      const char* file_name     = STD2_BUILTIN_FILE(),
      const char* function_name = STD2_BUILTIN_FUNCTION(),
      std::uint32_t line        = STD2_BUILTIN_LINE(),
      std::uint32_t column      = STD2_BUILTIN_COLUMN()) noexcept {
    source_location loc;
    loc.file_name_     = file_name;
    loc.function_name_ = function_name;
    loc.line_          = line;
    loc.column_        = column;
    return loc;
  }

  source_location() noexcept
    : file_name_(""), function_name_(""), line_(0), column_(0) {}

  const char*   file_name()     const noexcept { return file_name_; }
  const char*   function_name() const noexcept { return function_name_; }
  std::uint32_t line()          const noexcept { return line_; }
  std::uint32_t column()        const noexcept { return column_; }

private:
  const char*   file_name_;
  const char*   function_name_;
  std::uint32_t line_;
  std::uint32_t column_;
};

////////////////////////////////////////////////////////////////////////////////
// panic_error -- thrown when STD2_PANIC_THROWS is defined.

class panic_error : public std::runtime_error {
public:
  panic_error(panic_code code, const std::string& what, source_location loc)
    : std::runtime_error(what), code_(code), loc_(loc) {}

  panic_code             code()     const noexcept { return code_; }
  const source_location& location() const noexcept { return loc_; }

private:
  panic_code      code_;
  source_location loc_;
};

////////////////////////////////////////////////////////////////////////////////
// Panic functions

namespace detail {

[[noreturn]] STD2_NOINLINE
inline void panic_impl(panic_code code, const char* msg, std::size_t msg_len,
                       source_location loc) {
#if defined(STD2_PANIC_HANDLER)
  STD2_PANIC_HANDLER(code, msg, msg_len, loc);
  std::abort();  // handler must not return
#elif defined(STD2_PANIC_THROWS)
  throw panic_error(code, std::string(msg, msg_len), loc);
#else
  (void)code;
  std::fprintf(stderr, "%s:%u:%u\n%s\n%.*s\n",
               loc.file_name(),
               static_cast<unsigned>(loc.line()),
               static_cast<unsigned>(loc.column()),
               loc.function_name(),
               static_cast<int>(msg_len), msg);
  std::fflush(stderr);
  std::abort();
#endif
}

inline std::size_t cstrlen(const char* s) {
  std::size_t n = 0;
  while (s[n]) ++n;
  return n;
}

} // namespace detail

[[noreturn]]
inline void panic(const char* msg,
                  source_location loc = source_location::current()) {
  detail::panic_impl(panic_code::generic, msg, detail::cstrlen(msg), loc);
}

[[noreturn]]
inline void panic(const std::string& msg,
                  source_location loc = source_location::current()) {
  detail::panic_impl(panic_code::generic, msg.data(), msg.size(), loc);
}

[[noreturn]]
inline void panic_bounds(const char* msg,
                         source_location loc = source_location::current()) {
  detail::panic_impl(panic_code::bounds, msg, detail::cstrlen(msg), loc);
}

[[noreturn]]
inline void panic_utf(const char* msg,
                      source_location loc = source_location::current()) {
  detail::panic_impl(panic_code::utf, msg, detail::cstrlen(msg), loc);
}
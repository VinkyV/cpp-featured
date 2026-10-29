#ifndef CPP_FEATURED_HPP
#define CPP_FEATURED_HPP

#include <cstdio>
#include <cstdarg>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- io.h ---- */
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



template <typename... Args>
inline void println(const char* fmt, Args... args) {
    std::printf(fmt, args...);
    std::printf("\n");
}

inline void println(const char* s) {
  printf("%s\n", s);
}

inline void println(const std::string* s) {
  printf("%s\n", s->c_str());
}

inline void println(signed char x)
{
  printf("%hhd\n", x);
}

inline void println(short x)
{
  printf("%hd\n", x);
}

inline void println(int x)
{
  printf("%d\n", x);
}

inline void println(long int x)
{
  printf("%ld\n", x);
}

inline void println(long long int x)
{
  printf("%lld\n", x);
}

inline void println(unsigned char x)
{
  printf("%hhu\n", x);
}

inline void println(unsigned short x)
{
  printf("%hu\n", x);
}

inline void println(unsigned int x)
{
  printf("%u\n", x);
}

inline void println(long unsigned int x)
{
  printf("%lu\n", x);
}

inline void println(long long unsigned int x)
{
  printf("%llu\n", x);
}

inline void println(float x) {
  printf("%f\n", x);
}

inline void println(double x) {
  printf("%f\n", x);
}

inline void println(long double x) {
  printf("%Lf\n", x);
}

/* ---- panic.h ---- */


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

/* ---- string_view.h ---- */



////////////////////////////////////////////////////////////////////////////////
// UTF validation. Each returns the index of the first invalid code unit,
// or n if the whole range is valid.

namespace detail {

inline std::size_t verify_utf8(const unsigned char* s, std::size_t n) noexcept {
  std::size_t i = 0;
  while (i < n) {
    const unsigned char c = s[i];
    if (c < 0x80) { ++i; continue; }

    std::size_t len;
    if      (c >= 0xc2 && c <= 0xdf) len = 2;   // 0xc0/0xc1 are overlong
    else if (c >= 0xe0 && c <= 0xef) len = 3;
    else if (c >= 0xf0 && c <= 0xf4) len = 4;
    else return i;

    if (n - i < len) return i;
    for (std::size_t k = 1; k < len; ++k)
      if ((s[i + k] & 0xc0) != 0x80) return i;

    const unsigned char c2 = s[i + 1];
    if (c == 0xe0 && c2 < 0xa0) return i;        // overlong 3-byte
    if (c == 0xed && c2 > 0x9f) return i;        // UTF-16 surrogates
    if (c == 0xf0 && c2 < 0x90) return i;        // overlong 4-byte
    if (c == 0xf4 && c2 > 0x8f) return i;        // > U+10FFFF

    i += len;
  }
  return n;
}

inline std::size_t verify_utf(const char* p, std::size_t n) noexcept {
  return verify_utf8(reinterpret_cast<const unsigned char*>(p), n);
}

inline std::size_t verify_utf(const char16_t* s, std::size_t n) noexcept {
  std::size_t i = 0;
  while (i < n) {
    const char16_t c = s[i];
    if (c < 0xd800 || c >= 0xe000) { ++i; continue; }
    // Must be a high surrogate followed by a low surrogate.
    if ((c & 0xfc00) == 0xd800 && n - i >= 2 && (s[i + 1] & 0xfc00) == 0xdc00) {
      i += 2;
      continue;
    }
    return i;
  }
  return n;
}

inline std::size_t verify_utf(const char32_t* s, std::size_t n) noexcept {
  for (std::size_t i = 0; i < n; ++i) {
    const char32_t c = s[i];
    if (!(c < 0xd800 || (c > 0xdfff && c <= 0x10ffff))) return i;
  }
  return n;
}

#if defined(__cpp_char8_t)
inline std::size_t verify_utf(const char8_t* p, std::size_t n) noexcept {
  return verify_utf8(reinterpret_cast<const unsigned char*>(p), n);
}
#endif

inline std::size_t verify_wide(const wchar_t* p, std::size_t n,
                               std::integral_constant<std::size_t, 2>) noexcept {
  return verify_utf(reinterpret_cast<const char16_t*>(p), n);
}
inline std::size_t verify_wide(const wchar_t* p, std::size_t n,
                               std::integral_constant<std::size_t, 4>) noexcept {
  return verify_utf(reinterpret_cast<const char32_t*>(p), n);
}
inline std::size_t verify_utf(const wchar_t* p, std::size_t n) noexcept {
  return verify_wide(p, n,
      std::integral_constant<std::size_t, sizeof(wchar_t)>());
}

template<class CharT>
inline std::size_t length_of(const CharT* p) noexcept {
  std::size_t n = 0;
  while (p[n] != CharT(0)) ++n;
  return n;
}

} // namespace detail

////////////////////////////////////////////////////////////////////////////////
// basic_string_view

struct no_utf_check {};

template<class CharT>
class basic_string_view {
public:
  using value_type      = CharT;
  using pointer         = const CharT*;
  using const_pointer   = const CharT*;
  using reference       = const CharT&;
  using const_reference = const CharT&;
  using const_iterator  = const CharT*;
  using iterator        = const CharT*;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  static constexpr size_type npos = static_cast<size_type>(-1);

  // Unchecked: caller guarantees [p, p+n) is valid UTF.
  basic_string_view(const CharT* p, size_type n, no_utf_check) noexcept
    : p_(p), n_(n) {}

  // Checked: panics on malformed UTF.
  basic_string_view(const CharT* p, size_type n) : p_(p), n_(n) {
    if (detail::verify_utf(p, n) != n)
      panic_utf("invalid utf detected");
  }

  // Checked, from a NUL-terminated string (including string literals).
  basic_string_view(const CharT* cstr)
    : basic_string_view(cstr, detail::length_of(cstr)) {}

  template<class Traits, class Alloc>
  basic_string_view(const std::basic_string<CharT, Traits, Alloc>& s)
    : basic_string_view(s.data(), s.size()) {}

  // Non-panicking validation. Returns true and fills `out` on success.
  static bool try_from(const CharT* p, size_type n,
                       basic_string_view& out) noexcept {
    if (detail::verify_utf(p, n) != n) return false;
    out = basic_string_view(p, n, no_utf_check{});
    return true;
  }

  // Returns the index of the first invalid code unit, or n if all valid.
  static size_type verify(const CharT* p, size_type n) noexcept {
    return detail::verify_utf(p, n);
  }

  const CharT* data()  const noexcept { return p_; }
  size_type    size()  const noexcept { return n_; }
  size_type    length() const noexcept { return n_; }
  bool         empty() const noexcept { return n_ == 0; }

  const_iterator begin() const noexcept { return p_; }
  const_iterator end()   const noexcept { return p_ + n_; }

  // Bounds-checked access, like the original's operator[] on slices.
  const CharT& operator[](size_type i) const {
    if (i >= n_) panic_bounds("string_view subscript is out-of-bounds");
    return p_[i];
  }

  bool operator==(const basic_string_view& rhs) const noexcept {
    return n_ == rhs.n_ &&
           (n_ == 0 || std::memcmp(p_, rhs.p_, sizeof(CharT) * n_) == 0);
  }
  bool operator!=(const basic_string_view& rhs) const noexcept {
    return !(*this == rhs);
  }

private:
  const CharT* p_;
  size_type    n_;
};

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type
basic_string_view<CharT>::npos;

// C++-style typedefs.
using string_view    = basic_string_view<char>;
using wstring_view   = basic_string_view<wchar_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;

// Rust-style typedefs.
using str    = basic_string_view<char>;
using wstr   = basic_string_view<wchar_t>;
using u16str = basic_string_view<char16_t>;
using u32str = basic_string_view<char32_t>;

#if defined(__cpp_char8_t)
using u8string_view = basic_string_view<char8_t>;
using u8str         = basic_string_view<char8_t>;
#endif

////////////////////////////////////////////////////////////////////////////////
// basic_string -- owning, always-valid UTF.

template<class CharT>
class basic_string {
public:
  using value_type = CharT;
  using size_type  = std::size_t;
  using view_type  = basic_string_view<CharT>;

  static_assert(std::is_trivially_destructible<CharT>::value,
                "character type must be trivially destructible");

  basic_string() noexcept : p_(nullptr), size_(0), capacity_(0) {}

  // Explicit: the user should know this allocates.
  explicit basic_string(view_type sv)
    : p_(nullptr), size_(0), capacity_(0) {
    assign_raw(sv.data(), sv.size());
  }

  explicit basic_string(const CharT* cstr) : basic_string(view_type(cstr)) {}

  basic_string(const basic_string& rhs)
    : p_(nullptr), size_(0), capacity_(0) {
    assign_raw(rhs.p_, rhs.size_);
  }

  basic_string(basic_string&& rhs) noexcept
    : p_(rhs.p_), size_(rhs.size_), capacity_(rhs.capacity_) {
    rhs.p_ = nullptr;
    rhs.size_ = rhs.capacity_ = 0;
  }

  basic_string& operator=(const basic_string& rhs) {
    if (this != &rhs) {
      size_ = 0;
      assign_raw(rhs.p_, rhs.size_);
    }
    return *this;
  }

  basic_string& operator=(basic_string&& rhs) noexcept {
    if (this != &rhs) {
      ::operator delete(p_);
      p_ = rhs.p_;
      size_ = rhs.size_;
      capacity_ = rhs.capacity_;
      rhs.p_ = nullptr;
      rhs.size_ = rhs.capacity_ = 0;
    }
    return *this;
  }

  ~basic_string() { ::operator delete(p_); }

  view_type str() const noexcept {
    // Contents were validated on the way in, so skip re-checking.
    return view_type(p_, size_, no_utf_check{});
  }
  operator view_type() const noexcept { return str(); }

  const CharT* data()     const noexcept { return p_; }
  size_type    size()     const noexcept { return size_; }
  size_type    length()   const noexcept { return size_; }
  size_type    capacity() const noexcept { return capacity_; }
  bool         empty()    const noexcept { return size_ == 0; }

  void reserve(size_type n) {
    if (n <= capacity_) return;
    CharT* p = static_cast<CharT*>(::operator new(n * sizeof(CharT)));
    if (size_) std::memcpy(p, p_, size_ * sizeof(CharT));
    ::operator delete(p_);
    p_ = p;
    capacity_ = n;
  }

  void append(view_type rhs) {
    if (rhs.empty()) return;
    const size_type len = size_ + rhs.size();
    if (len > capacity_) {
      // Geometric growth; handles `s.append(s.str())` since rhs is copied
      // before the old buffer is freed only when we reallocate below.
      size_type ncap = capacity_ ? capacity_ * 2 : len;
      if (ncap < len) ncap = len;
      CharT* p = static_cast<CharT*>(::operator new(ncap * sizeof(CharT)));
      if (size_) std::memcpy(p, p_, size_ * sizeof(CharT));
      std::memcpy(p + size_, rhs.data(), rhs.size() * sizeof(CharT));
      ::operator delete(p_);
      p_ = p;
      capacity_ = ncap;
    } else {
      std::memmove(p_ + size_, rhs.data(), rhs.size() * sizeof(CharT));
    }
    size_ = len;
  }

  basic_string operator+(view_type rhs) const {
    basic_string s(*this);
    s.append(rhs);
    return s;
  }

  bool operator==(const basic_string& rhs) const noexcept {
    return str() == rhs.str();
  }
  bool operator!=(const basic_string& rhs) const noexcept {
    return !(*this == rhs);
  }

private:
  void assign_raw(const CharT* src, size_type n) {
    if (n == 0) { size_ = 0; return; }
    if (n > capacity_) {
      CharT* p = static_cast<CharT*>(::operator new(n * sizeof(CharT)));
      ::operator delete(p_);
      p_ = p;
      capacity_ = n;
    }
    std::memcpy(p_, src, n * sizeof(CharT));
    size_ = n;
  }

  CharT*    p_;
  size_type size_;
  size_type capacity_;
};

using string    = basic_string<char>;
using wstring   = basic_string<wchar_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;

#if defined(__cpp_char8_t)
using u8string = basic_string<char8_t>;
#endif

////////////////////////////////////////////////////////////////////////////////
// User-defined literals. (User literal suffixes must start with '_'.)
//
//   using namespace std2::literals::string_literals;
//   auto v = "héllo"_sv2;   // std2::string_view
//   auto s = "héllo"_s2;    // std2::string

namespace literals {
namespace string_literals {

inline string_view    operator"" _sv2(const char* p, std::size_t n)     { return string_view(p, n); }
inline wstring_view   operator"" _sv2(const wchar_t* p, std::size_t n)  { return wstring_view(p, n); }
inline u16string_view operator"" _sv2(const char16_t* p, std::size_t n) { return u16string_view(p, n); }
inline u32string_view operator"" _sv2(const char32_t* p, std::size_t n) { return u32string_view(p, n); }

inline string    operator"" _s2(const char* p, std::size_t n)     { return string(string_view(p, n)); }
inline wstring   operator"" _s2(const wchar_t* p, std::size_t n)  { return wstring(wstring_view(p, n)); }
inline u16string operator"" _s2(const char16_t* p, std::size_t n) { return u16string(u16string_view(p, n)); }
inline u32string operator"" _s2(const char32_t* p, std::size_t n) { return u32string(u32string_view(p, n)); }

#if defined(__cpp_char8_t)
inline u8string_view operator"" _sv2(const char8_t* p, std::size_t n) { return u8string_view(p, n); }
inline u8string      operator"" _s2(const char8_t* p, std::size_t n)  { return u8string(u8string_view(p, n)); }
#endif

} // namespace string_literals
} // namespace literals

/* ---- temp_alloc.h ---- */
/*
 * temp_alloc.h - C3-style temporary allocator for C and C++ (single header)
 *
 * tmalloc() works like malloc(), but memory comes from a thread-local arena
 * and is never freed individually. Instead you release everything allocated
 * since a point in time by resetting to a mark (the C3 `@pool` idea).
 *
 * Setup (stb style): in exactly ONE .c/.cpp file, before including:
 *
 *     #define TEMP_ALLOC_IMPLEMENTATION
 *     #include "temp_alloc.h"
 *
 * Every other file just includes the header.
 *
 * Usage (C or C++):
 *
 *     TEMP_POOL {                         // everything inside is freed at '}'
 *         char *s = tprintf("%s-%d", name, id);
 *         int  *v = (int *)tmalloc(n * sizeof(int));
 *         ...
 *     }
 *
 *     C++ only, RAII:  { temp_scope scope; char *s = tstrdup("x"); }
 *
 * Notes:
 *  - TEMP_POOL: leaving it with break/return/goto skips the reset (memory is
 *    then released by the next enclosing reset). In C++ prefer temp_scope.
 *  - Memory is 16-byte aligned. The arena is per thread. Pointers stay valid
 *    until the pool they were allocated in ends (the arena never moves).
 *  - Never pass tmalloc'd memory to free(). Call temp_free_all() to give
 *    all blocks back to the system (e.g. before a thread exits).
 *  - C++ needs C++11 (thread_local).
 *
 * Optional settings (define before the IMPLEMENTATION include):
 *     TEMP_BLOCK_SIZE   bytes per arena block (default 64 KiB)
 *     TEMP_MALLOC/TEMP_FREE   backing allocator (default malloc/free)
 */


#ifdef __cplusplus
extern "C" {
#endif

typedef struct temp_block_ temp_block_;

typedef struct temp_mark_t {
    temp_block_ *block;
    size_t used;
} temp_mark_t;

#if defined(__GNUC__) || defined(__clang__)
#define TEMP_PRINTF_(f, a) __attribute__((format(printf, f, a)))
#else
#define TEMP_PRINTF_(f, a)
#endif

/* malloc / calloc / realloc / strdup / printf, all from the temp arena */
void *tmalloc(size_t size);
void *tcalloc(size_t count, size_t size);
void *trealloc(void *ptr, size_t old_size, size_t new_size); /* ptr may be NULL */
char *tstrdup(const char *s);
char *tprintf(const char *fmt, ...) TEMP_PRINTF_(1, 2);
char *tvprintf(const char *fmt, va_list ap);

/* Pools: remember a position, later release everything allocated after it */
temp_mark_t temp_mark(void);
void temp_reset(temp_mark_t mark);
void temp_clear(void);     /* release everything, keep blocks for reuse */
void temp_free_all(void);  /* give all blocks back to the system */

#ifdef __cplusplus
}
#endif

/* TEMP_POOL { ... }  -- runs the block once, then resets the arena to where it was */
#define TEMP_POOL                                                       \
    for (temp_mark_t temp_m_ = temp_mark(), *temp_once_ = &temp_m_;     \
         temp_once_; temp_once_ = NULL, temp_reset(temp_m_))

#ifdef __cplusplus
struct temp_scope {
    temp_mark_t mark_;
    temp_scope() : mark_(temp_mark()) {}
    ~temp_scope() { temp_reset(mark_); }
    temp_scope(const temp_scope &) = delete;
    temp_scope &operator=(const temp_scope &) = delete;
};
#endif

/* ======================================================================= */
#ifdef TEMP_ALLOC_IMPLEMENTATION

#ifndef TEMP_BLOCK_SIZE
#define TEMP_BLOCK_SIZE (64 * 1024)
#endif
#ifndef TEMP_MALLOC
#define TEMP_MALLOC malloc
#endif
#ifndef TEMP_FREE
#define TEMP_FREE free
#endif

#ifndef TEMP_TLS
#  if defined(__cplusplus)
#    define TEMP_TLS thread_local
#  elif defined(_MSC_VER)
#    define TEMP_TLS __declspec(thread)
#  elif defined(__GNUC__) || defined(__clang__)
#    define TEMP_TLS __thread
#  elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#    define TEMP_TLS _Thread_local
#  else
#    define TEMP_TLS
#  endif
#endif

#define TEMP_ALIGN 16
#define TEMP_ROUND(n) (((n) + (TEMP_ALIGN - 1)) & ~(size_t)(TEMP_ALIGN - 1))
#define TEMP_MAX_REQUEST (((size_t)-1) / 2)

#ifdef __cplusplus
extern "C" {
#endif

struct temp_block_ {
    temp_block_ *next;  /* next block in the chain (spare when not current) */
    size_t cap;         /* usable bytes after the header */
    size_t used;
};

#define TEMP_HDR TEMP_ROUND(sizeof(struct temp_block_))

static TEMP_TLS temp_block_ *temp_first_;
static TEMP_TLS temp_block_ *temp_cur_;

static unsigned char *temp_data_(temp_block_ *b) {
    return (unsigned char *)b + TEMP_HDR;
}

static temp_block_ *temp_new_block_(size_t min_cap) {
    size_t cap = TEMP_ROUND((size_t)TEMP_BLOCK_SIZE);
    temp_block_ *b;
    if (cap < min_cap) cap = min_cap;
    b = (temp_block_ *)TEMP_MALLOC(TEMP_HDR + cap);
    if (!b) return NULL;
    b->next = NULL;
    b->cap = cap;
    b->used = 0;
    return b;
}

void *tmalloc(size_t size) {
    temp_block_ *b = temp_cur_;
    void *p;

    if (size > TEMP_MAX_REQUEST) return NULL;
    size = TEMP_ROUND(size ? size : 1);

    if (!b) {
        b = temp_new_block_(size);
        if (!b) return NULL;
        temp_first_ = temp_cur_ = b;
    } else if (b->cap - b->used < size) {
        temp_block_ *n = b->next;
        if (!n || n->cap < size) {
            temp_block_ *nb = temp_new_block_(size);
            if (!nb) return NULL;
            nb->next = n;   /* keep the too-small spare chained after it */
            b->next = nb;
            n = nb;
        }
        n->used = 0;
        temp_cur_ = b = n;
    }

    p = temp_data_(b) + b->used;
    b->used += size;
    return p;
}

void *tcalloc(size_t count, size_t size) {
    void *p;
    if (size && count > TEMP_MAX_REQUEST / size) return NULL;
    p = tmalloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}

void *trealloc(void *ptr, size_t old_size, size_t new_size) {
    temp_block_ *b = temp_cur_;
    size_t o, n;
    void *np;

    if (!ptr) return tmalloc(new_size);
    if (new_size > TEMP_MAX_REQUEST) return NULL;

    o = TEMP_ROUND(old_size ? old_size : 1);
    n = TEMP_ROUND(new_size ? new_size : 1);

    /* last allocation in the current block: grow or shrink in place */
    if (b && b->used >= o && (unsigned char *)ptr == temp_data_(b) + b->used - o) {
        if (n <= o) { b->used -= o - n; return ptr; }
        if (b->cap - b->used >= n - o) { b->used += n - o; return ptr; }
    }

    np = tmalloc(new_size);
    if (np) memcpy(np, ptr, old_size < new_size ? old_size : new_size);
    return np;
}

char *tstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)tmalloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

char *tvprintf(const char *fmt, va_list ap) {
    va_list ap2;
    char buf[256];
    int n;
    char *p;

    /* common case: result fits the stack buffer, so only one formatting pass */
    va_copy(ap2, ap);
    n = vsnprintf(buf, sizeof buf, fmt, ap2);
    va_end(ap2);
    if (n < 0) return NULL;

    p = (char *)tmalloc((size_t)n + 1);
    if (!p) return NULL;
    if ((size_t)n < sizeof buf) memcpy(p, buf, (size_t)n + 1);
    else vsnprintf(p, (size_t)n + 1, fmt, ap);
    return p;
}

char *tprintf(const char *fmt, ...) {
    va_list ap;
    char *p;
    va_start(ap, fmt);
    p = tvprintf(fmt, ap);
    va_end(ap);
    return p;
}

temp_mark_t temp_mark(void) {
    temp_mark_t m;
    m.block = temp_cur_;
    m.used = temp_cur_ ? temp_cur_->used : 0;
    return m;
}

void temp_reset(temp_mark_t m) {
    if (m.block) {
        temp_cur_ = m.block;
        m.block->used = m.used;
    } else if (temp_first_) {   /* mark taken before the first allocation */
        temp_cur_ = temp_first_;
        temp_first_->used = 0;
    }
}

void temp_clear(void) {
    temp_mark_t m;
    m.block = NULL;
    m.used = 0;
    temp_reset(m);
}

void temp_free_all(void) {
    temp_block_ *b = temp_first_;
    while (b) {
        temp_block_ *next = b->next;
        TEMP_FREE(b);
        b = next;
    }
    temp_first_ = temp_cur_ = NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* TEMP_ALLOC_IMPLEMENTATION */

#endif /* CPP_FEATURED_HPP */

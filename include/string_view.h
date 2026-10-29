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

#include "panic.h"

#include <cstddef>
#include <cstring>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

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
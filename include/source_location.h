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

class source_location {
public:
  static source_location current(
    char const* file_name     = __builtin_FILE(),
    char const* function_name = __builtin_FUNCTION(),
    uint32_t    line          = __builtin_LINE(),
    uint32_t    column        = __builtin_COLUMN()
  ) noexcept {
    source_location loc;
    loc._file_name     = file_name;
    loc._function_name = function_name;
    loc._line          = line;
    loc._column        = column;
    return loc;
  }

  source_location() noexcept : _file_name(""), _function_name(""), _line(0), _column(0) {}

  const char*   file_name()     const noexcept { return _file_name; }
  const char*   function_name() const noexcept { return _function_name; }
  std::uint32_t line()          const noexcept { return _line; }
  std::uint32_t column()        const noexcept { return _column; }


private:
  char const* _file_name;
  char const* _function_name;
  uint32_t _line;
  uint32_t _column;
};
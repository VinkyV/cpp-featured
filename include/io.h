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

#include <cstdio>
#include <cstdarg>
#include <string>

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
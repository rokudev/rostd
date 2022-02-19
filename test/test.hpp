// clang-format off

#ifndef ROSTD_TEST_HPP
#define ROSTD_TEST_HPP

#undef NDEBUG
#include <cassert>

#define assert_throw(expression) do { try { expression; assert(false); } catch(...) {}; } while(false)
#define assert_nothrow(expression) do { try { expression; } catch (...) { assert(false); } } while(false)
#define assert_throw_with(expression,exception) do { try { expression; assert(false); } catch(const exception&) {}; } while(false)

#endif // ROSTD_TEST_HPP

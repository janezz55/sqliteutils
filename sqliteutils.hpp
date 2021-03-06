/*
** This is free and unencumbered software released into the public domain.
**
** Anyone is free to copy, modify, publish, use, compile, sell, or
** distribute this software, either in source code form or as a compiled
** binary, for any purpose, commercial or non-commercial, and by any
** means.
**
** In jurisdictions that recognize copyright laws, the author or authors
** of this software dedicate any and all copyright interest in the
** software to the public domain. We make this dedication for the benefit
** of the public at large and to the detriment of our heirs and
** successors. We intend this dedication to be an overt act of
** relinquishment in perpetuity of all present and future rights to this
** software under copyright law.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
** OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
** OTHER DEALINGS IN THE SOFTWARE.
**
** For more information, please refer to <http://unlicense.org>
*/

#ifndef SQLITEUTILS_HPP
# define SQLITEUTILS_HPP
# pragma once

#include <cassert>

#include <memory>

#include <optional>

#include <string>

#include <string_view>

#include <type_traits>

#include <utility>

#include "sqlite3.h"

namespace squ
{

enum encoding
{
  UTF8 = SQLITE_UTF8,
  UTF16LE = SQLITE_UTF16LE,
  UTF16BE = SQLITE_UTF16BE,
  UTF16 = SQLITE_UTF16,
  UTF16_ALIGNED = SQLITE_UTF16_ALIGNED
};

enum store
{
  STATIC,
  TRANSIENT
};

template <enum store S = TRANSIENT>
struct blobpair
{
  void const* const first;
  sqlite3_uint64 const second;
};

template <enum store = TRANSIENT, enum encoding = UTF8>
struct charpair
{
  char const* const first;
  sqlite3_uint64 const second;
};

template <enum store = TRANSIENT, enum encoding = UTF16>
struct char16pair
{
  char16_t const* const first;
  sqlite3_uint64 const second;
};

namespace detail
{

struct sqlite3_deleter
{
  void operator()(sqlite3* const p) const noexcept
  {
    sqlite3_close_v2(p);
  }
};

struct sqlite3_stmt_deleter
{
  void operator()(sqlite3_stmt* const p) const noexcept
  {
    sqlite3_finalize(p);
  }
};

template <int I>
inline auto set(sqlite3_stmt* const s, std::nullptr_t) noexcept
{
  return sqlite3_bind_null(s, I);
}

template <int I, typename T>
inline std::enable_if_t<
  std::is_floating_point<T>{},
  decltype(sqlite3_bind_double({}, I, std::declval<T>()))
>
set(sqlite3_stmt* const s, T const v) noexcept
{
  return sqlite3_bind_double(s, I, v);
}

template <int I, typename T>
inline std::enable_if_t<
  std::is_integral<T>{} && (sizeof(T) <= sizeof(int)),
  decltype(sqlite3_bind_int({}, I, std::declval<T>()))
>
set(sqlite3_stmt* const s, T const v) noexcept
{
  return sqlite3_bind_int(s, I, v);
}

template <int I, typename T>
inline std::enable_if_t<
  std::is_integral<T>{} && (sizeof(T) > sizeof(int)),
  decltype(sqlite3_bind_int64({}, I, std::declval<T>()))
>
set(sqlite3_stmt* const s, T const v) noexcept
{
  return sqlite3_bind_int64(s, I, v);
}

//
template <int I>
inline auto set(sqlite3_stmt* const s, blobpair<STATIC> const& v) noexcept
{
  return v.first ?
    sqlite3_bind_blob64(s, I, v.first, v.second, SQLITE_STATIC) :
    sqlite3_bind_zeroblob64(s, I, v.second);
}

template <int I>
inline auto set(sqlite3_stmt* const s, blobpair<TRANSIENT> const& v) noexcept
{
  return v.first ?
    sqlite3_bind_blob64(s, I, v.first, v.second, SQLITE_TRANSIENT) :
    sqlite3_bind_zeroblob64(s, I, v.second);
}

//
template <int I, typename T>
inline std::enable_if_t<
  std::is_same_v<T, char>,
  decltype(sqlite3_bind_text64({}, I, {}, -1, SQLITE_TRANSIENT, SQLITE_UTF8))
>
set(sqlite3_stmt* const s, T const* const& v) noexcept
{
  return sqlite3_bind_text64(s, I, v, -1, SQLITE_TRANSIENT, SQLITE_UTF8);
}

template <int I, std::size_t N>
inline auto set(sqlite3_stmt* const s, char const (&v)[N]) noexcept
{
  return sqlite3_bind_text64(s, I, v, N - 1, SQLITE_STATIC, SQLITE_UTF8);
}

template <int I, enum encoding E>
inline auto set(sqlite3_stmt* const s, charpair<STATIC, E> const& v) noexcept
{
  return sqlite3_bind_text64(s, I, v.first, v.second, SQLITE_STATIC, E);
}

template <int I, enum encoding E>
inline auto set(sqlite3_stmt* const s,
  charpair<TRANSIENT, E> const& v) noexcept
{
  return sqlite3_bind_text64(s, I, v.first, v.second, SQLITE_TRANSIENT, E);
}

template <int I>
inline auto set(sqlite3_stmt* const s, std::string const& v) noexcept
{
  return sqlite3_bind_text64(s, I, v.data(), v.size(), SQLITE_TRANSIENT,
    SQLITE_UTF8);
}

template <int I>
inline auto set(sqlite3_stmt* const s, std::string_view const& v) noexcept
{
  return sqlite3_bind_text64(s, I, v.data(), v.size(), SQLITE_TRANSIENT,
    SQLITE_UTF8);
}

//
template <int I, typename T>
inline std::enable_if_t<
  std::is_same_v<T, char16_t>,
  decltype(sqlite3_bind_text64({}, I, {}, -1, SQLITE_TRANSIENT, SQLITE_UTF16))
>
set(sqlite3_stmt* const s, T const* const& v) noexcept
{
  return sqlite3_bind_text64(s, I, v, -1, SQLITE_TRANSIENT, SQLITE_UTF16);
}

template <int I, std::size_t N>
inline auto set(sqlite3_stmt* const s, char16_t const (&v)[N]) noexcept
{
  return sqlite3_bind_text64(s, I, reinterpret_cast<char const*>(&*v),
    (N - 1) * sizeof(char16_t), SQLITE_STATIC, SQLITE_UTF16);
}

template <int I, enum encoding E>
inline auto set(sqlite3_stmt* const s,
  char16pair<STATIC, E> const& v) noexcept
{
  return sqlite3_bind_text64(s, I, reinterpret_cast<char const*>(v.first),
    v.second * sizeof(char16_t), SQLITE_STATIC, E);
}

template <int I, enum encoding E>
inline auto set(sqlite3_stmt* const s,
  char16pair<TRANSIENT> const& v) noexcept
{
  return sqlite3_bind_text64(s, I, reinterpret_cast<char const*>(v.first),
    v.second * sizeof(char16_t), SQLITE_TRANSIENT, E);
}

template <int I>
inline auto set(sqlite3_stmt* const s, std::u16string const& v) noexcept
{
  return sqlite3_bind_text64(s, I, reinterpret_cast<char const*>(v.data()),
    v.size() * sizeof(char16_t), SQLITE_TRANSIENT, SQLITE_UTF16);
}

template <int I>
inline auto set(sqlite3_stmt* const s, std::u16string_view const& v) noexcept
{
  return sqlite3_bind_text64(s, I, reinterpret_cast<char const*>(v.data()),
    v.size() * sizeof(char16_t), SQLITE_TRANSIENT, SQLITE_UTF16);
}

//
template <int I, std::size_t ...Is, typename A, typename ...B>
auto set(sqlite3_stmt* const s, std::index_sequence<Is...>, A&& a,
  B&& ...b) noexcept(
  noexcept(
    (
      set<I>(s, std::forward<A>(a)),
      (set<I + Is + 1>(s, std::forward<B>(b)), ...)
    )
  )
)
{
  int r(set<I>(s, std::forward<A>(a)));

  return (
    (
      SQLITE_OK == r ? r = set<I + Is + 1>(s, std::forward<B>(b)) : r
    ),
    ...
  );
}

}

using shared_db_t = std::shared_ptr<sqlite3>;

using unique_db_t = std::unique_ptr<sqlite3, detail::sqlite3_deleter>;

template <typename T>
using remove_cvr_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
using is_db_t =
  std::integral_constant<
    bool,
    std::is_same_v<remove_cvr_t<T>, shared_db_t> ||
    std::is_same_v<remove_cvr_t<T>, unique_db_t>
  >;

template <typename T>
inline constexpr bool is_db_v{is_db_t<T>{}};

using shared_stmt_t = std::shared_ptr<sqlite3_stmt>;

using unique_stmt_t = std::unique_ptr<sqlite3_stmt,
  detail::sqlite3_stmt_deleter
>;

template <typename T>
using is_stmt_t =
  std::integral_constant<
    bool,
    std::is_same_v<remove_cvr_t<T>, shared_stmt_t> ||
    std::is_same_v<remove_cvr_t<T>, unique_stmt_t>
  >;

template <typename T>
inline constexpr bool is_stmt_v{is_stmt_t<T>{}};

namespace detail
{

template <typename>
struct is_charpair : std::false_type {};

template <enum store A, enum encoding B>
struct is_charpair<charpair<A, B>> : std::true_type {};

template <typename>
struct is_char16pair : std::false_type {};

template <enum store A, enum encoding B>
struct is_char16pair<char16pair<A, B>> : std::true_type {};

template <typename>
struct is_std_pair : std::false_type {};

template <class T1, class T2>
struct is_std_pair<std::pair<T1, T2>> : std::true_type {};

template <typename>
struct is_std_tuple : std::false_type {};

template <class ...A>
struct is_std_tuple<std::tuple<A...>> : std::true_type {};

}

//errmsg//////////////////////////////////////////////////////////////////////
inline auto errmsg(sqlite3* const db) noexcept
{
  return sqlite3_errmsg(db);
}

template <typename D, typename = std::enable_if_t<is_db_v<D>>>
inline auto errmsg(D const& db) noexcept
{
  return sqlite3_errmsg(db.get());
}

inline auto errmsg16(sqlite3* const db) noexcept
{
  return sqlite3_errmsg16(db);
}

template <typename D, typename = std::enable_if_t<is_db_v<D>>>
inline auto errmsg16(D const& db) noexcept
{
  return sqlite3_errmsg16(db.get());
}

//set/////////////////////////////////////////////////////////////////////////
template <int I = 1, typename A>
inline auto set(sqlite3_stmt* const s, A&& a) noexcept(
  noexcept(
    detail::set<I>(s, std::declval<A>())
  )
)
{
  return detail::set<I>(s, std::forward<A>(a));
}

template <int I = 1, typename ...A>
inline auto set(sqlite3_stmt* const s, A&& ...a) noexcept(
  noexcept(
    detail::set<I>(s,
      std::make_index_sequence<sizeof...(A) - 1>(),
      std::declval<A>()...
    )
  )
)
{
  return detail::set<I>(s,
    std::make_index_sequence<sizeof...(A) - 1>(),
    std::forward<A>(a)...
  );
}

template <int I = 1, typename S, typename ...A,
  typename = std::enable_if_t<is_stmt_v<S>>
>
inline auto set(S const& s, A&& ...args) noexcept(
  noexcept(set<I>(s.get(), std::forward<A>(args)...))
)
{
  return set<I>(s.get(), std::forward<A>(args)...);
}

//rset////////////////////////////////////////////////////////////////////////
template <int I = 1, typename ...A>
inline auto rset(sqlite3_stmt* const s, A&& ...a) noexcept(
  noexcept(set(s, std::forward<A>(a)...)))
{
  auto const r(sqlite3_reset(s));

  return SQLITE_OK == r ? set<I>(s, std::forward<A>(a)...) : r;
}

template <int I = 1, typename S, typename ...A,
  typename = std::enable_if_t<is_stmt_v<S>>
>
inline auto rset(S const& s, A&& ...args) noexcept(
  noexcept(rset<I>(s.get(), std::forward<A>(args)...))
)
{
  return rset<I>(s.get(), std::forward<A>(args)...);
}

//make_unique/////////////////////////////////////////////////////////////////
inline auto make_unique(sqlite3* const db, std::string_view const& sv,
  unsigned const fl = 0) noexcept
{
  sqlite3_stmt* s;

  auto const r(sqlite3_prepare_v3(db, sv.data(), sv.size(), fl, &s, nullptr));
  assert(SQLITE_OK == r);

  return SQLITE_OK == r ? unique_stmt_t(s) : unique_stmt_t();
}

template <std::size_t N>
inline auto make_unique(sqlite3* const db, char const (&a)[N],
  unsigned const fl = 0) noexcept(noexcept(make_unique(db, {a, N}, fl)))
{
  return make_unique(db, {a, N}, fl);
}

// forwarders
template <typename D, typename ...A, typename = std::enable_if_t<is_db_v<D>>>
inline auto make_unique(D const& db, A&& ...args) noexcept(
  noexcept(make_unique(db.get(), std::forward<A>(args)...))
)
{
  return make_unique(db.get(), std::forward<A>(args)...);
}

//make_shared/////////////////////////////////////////////////////////////////
inline auto make_shared(sqlite3* const db, std::string_view const& sv,
  unsigned const fl = 0) noexcept
{
  sqlite3_stmt* s;

  auto const r(sqlite3_prepare_v3(db, sv.data(), sv.size(), fl, &s, nullptr));
  assert(SQLITE_OK == r);

  return SQLITE_OK == r ?
    shared_stmt_t(s, detail::sqlite3_stmt_deleter()) :
    shared_stmt_t();
}

template <std::size_t N>
inline auto make_shared(sqlite3* const db, char const (&a)[N],
  unsigned const fl = 0) noexcept(noexcept(make_shared(db, {a, N}, fl)))
{
  return make_shared(db, {a, N}, fl);
}

// forwarders
template <typename D, typename ...A, typename = std::enable_if_t<is_db_v<D>>>
inline auto make_shared(D const& db, A&& ...args) noexcept(
  noexcept(make_shared(db.get(), std::forward<A>(args)...))
)
{
  return make_shared(db.get(), std::forward<A>(args)...);
}

//exec////////////////////////////////////////////////////////////////////////
template <int I = 1>
inline auto exec(sqlite3_stmt* const s) noexcept
{
  return sqlite3_step(s);
}

template <int I = 1, typename ...A>
inline auto exec(sqlite3_stmt* const s, A&& ...args) noexcept
{
  auto const r(set<I>(s, std::forward<A>(args)...));

  return SQLITE_OK == r ? sqlite3_step(s) : r;
}

template <int I = 1>
inline auto rexec(sqlite3_stmt* const s) noexcept
{
  auto const r(sqlite3_reset(s));

  return SQLITE_OK == r ? sqlite3_step(s) : r;
}

template <int I = 1, typename ...A>
inline auto rexec(sqlite3_stmt* const s, A&& ...args) noexcept
{
  auto const r(sqlite3_reset(s));

  return SQLITE_OK == r ? exec<I>(s, std::forward<A>(args)...) : r;
}

// forwarders
template <int I = 1, typename S, typename ...A>
inline std::enable_if_t<is_stmt_v<S>, decltype(exec<I>({}))>
exec(S const& s, A&& ...args) noexcept(
  noexcept(exec<I>(s.get(), std::forward<A>(args)...)))
{
  return exec<I>(s.get(), std::forward<A>(args)...);
}

template <int I = 1, typename S, typename ...A,
  typename = std::enable_if_t<is_stmt_v<S>>
>
inline auto rexec(S const& s, A&& ...args) noexcept(
  noexcept(rexec<I>(s.get(), std::forward<A>(args)...)))
{
  return rexec<I>(s.get(), std::forward<A>(args)...);
}

template <typename A, typename ...B>
inline auto exec(sqlite3* const db, A&& a, B&& ...args) noexcept(
  noexcept(
    exec(
      make_unique(db, std::forward<A>(a)), std::forward<B>(args)...
    )
  )
)
{
  return exec(
    make_unique(db, std::forward<A>(a)), std::forward<B>(args)...
  );
}

template <typename D, typename ...A, typename = std::enable_if_t<is_db_v<D>>>
inline auto exec(D const& db, A&& ...args) noexcept(
  noexcept(exec(db.get(), std::forward<A>(args)...))
)
{
  return exec(db.get(), std::forward<A>(args)...);
}

//execmulti///////////////////////////////////////////////////////////////////
template <typename T, typename = std::enable_if_t<std::is_same_v<T, char>>>
inline auto execmulti(sqlite3* const db, T const* const& a) noexcept
{
  return sqlite3_exec(db, a, nullptr, nullptr, nullptr);
}

template <std::size_t N>
inline auto execmulti(sqlite3* const db, char const (&a)[N]) noexcept
{
  assert(N && !a[N]);
  return sqlite3_exec(db, a, nullptr, nullptr, nullptr);
}

inline auto execmulti(sqlite3* const db, std::string const& a) noexcept
{
  return execmulti(db, a.c_str());
}

inline auto execmulti(sqlite3* const db, std::string_view const& a) noexcept
{
  assert(a.size() && !a[a.size()]);
  return execmulti(db, a.data());
}

template <typename D, typename ...A, typename = std::enable_if_t<is_db_v<D>>>
inline auto execmulti(D const& db, A&& ...args) noexcept(
  noexcept(execmulti(db.get(), std::forward<A>(args)...)))
{
  return execmulti(db.get(), std::forward<A>(args)...);
}

//get/////////////////////////////////////////////////////////////////////////
template <typename T>
inline std::enable_if_t<
  std::is_integral<T>{} &&
  (sizeof(T) <= sizeof(std::int32_t)),
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return sqlite3_column_int(s, i);
}

template <typename T>
inline std::enable_if_t<
  std::is_integral_v<T> &&
  (sizeof(T) > sizeof(std::int32_t)) &&
  (sizeof(T) <= sizeof(std::int64_t)),
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return sqlite3_column_int64(s, i);
}

template <typename T>
inline std::enable_if_t<
  std::is_floating_point_v<T>,
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return sqlite3_column_double(s, i);
}

template <typename T>
inline std::enable_if_t<
  std::is_same_v<T, char const*>,
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return reinterpret_cast<char const*>(sqlite3_column_text(s, i));
}

template <typename T>
inline std::enable_if_t<
  detail::is_charpair<T>{} ||
  detail::is_char16pair<T>{} ||
  std::is_same_v<T, std::string> ||
  std::is_same_v<T, std::string_view>,
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return {
    get<char const*>(s, i),
    unsigned(sqlite3_column_bytes(s, i))
  };
}

template <typename T>
inline std::enable_if_t<
  std::is_same_v<T, char16_t const*>,
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return static_cast<char16_t const*>(sqlite3_column_text16(s, i));
}

template <typename T>
inline std::enable_if_t<
  detail::is_charpair<T>{} ||
  detail::is_char16pair<T>{} ||
  std::is_same_v<T, std::u16string> ||
  std::is_same_v<T, std::u16string_view>,
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return {
    get<char16_t const*>(s, i),
    unsigned(sqlite3_column_bytes16(s, i) / sizeof(char16_t))
  };
}

template <typename T>
inline std::enable_if_t<
  std::is_same_v<T, void const*>,
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return sqlite3_column_blob(s, i);
}

template <typename T>
inline std::enable_if_t<
  std::is_same_v<T, blobpair<STATIC>> ||
  std::is_same_v<T, blobpair<TRANSIENT>>,
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return {
    get<void const*>(s, i),
    unsigned(sqlite3_column_bytes(s, i))
  };
}

namespace detail
{

template <typename A, typename ...B>
struct count_types :
  std::integral_constant<std::size_t, count_types<A>{} + count_types<B...>{}>
{
};

template <typename A>
struct count_types<A> : std::integral_constant<std::size_t, 1>
{
};

template <typename A, typename B>
struct count_types<std::pair<A, B>> :
  std::integral_constant<std::size_t, count_types<A>{} + count_types<B>{}>
{
};

template <typename A, typename ...B>
struct count_types<std::tuple<A, B...>> :
  std::integral_constant<std::size_t, count_types<A>{} + count_types<B...>{}>
{
};

template <std::size_t I, std::size_t S, typename A, typename ...B>
struct count_types_n : count_types_n<I - 1, S + count_types<A>{}, B...>
{
  static_assert(I > 0);
};

template <std::size_t S, typename A, typename ...B>
struct count_types_n<0, S, A, B...> : std::integral_constant<std::size_t, S>
{
};

template <typename T, std::size_t ...I>
T make_tuple(sqlite3_stmt* const s, int const i, std::index_sequence<I...>)
  noexcept(
    noexcept(
      T{
        std::tuple_element_t<I, T>(
          std::declval<std::tuple_element_t<I, T> const&>()
        )...
      }
    )
  )
{
  return T{
    get<std::tuple_element_t<I, T>>(s,
      i + count_types_n<I, 0, std::tuple_element_t<I, T>...>{}
    )...
  };
}

}

template <typename T>
inline std::enable_if_t<
  (detail::is_std_pair<T>{} ||
  detail::is_std_tuple<T>{}),
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept(
  noexcept(
    detail::make_tuple<T>(s,
      i,
      std::make_index_sequence<std::tuple_size_v<T>>()
    )
  )
)
{
  return detail::make_tuple<T>(s,
    i,
    std::make_index_sequence<std::tuple_size_v<T>>()
  );
}

template <typename ...A,
  typename = std::enable_if_t<bool(sizeof...(A) > 1)>
>
auto get(sqlite3_stmt* const s, int const i = 0) noexcept(
  noexcept(get<std::tuple<A...>>(s, i))
)
{
  return get<std::tuple<A...>>(s, i);
}

template <typename ...A, typename S,
  typename = std::enable_if_t<is_stmt_v<S>>
>
inline auto get(S const& s, int const i = 0) noexcept(
  noexcept(get<A...>(s.get(), i))
)
{
  return get<A...>(s.get(), i);
}

//execget/////////////////////////////////////////////////////////////////////
template <typename T, int I = 1, typename S, typename ...A>
inline auto execget(S&& s, int const i = 0, A&& ...args) noexcept(
  noexcept(exec<I>(std::forward<S>(s), std::forward<A>(args)...),
    get<T>(s, i)
  )
)
{
  auto const r(exec<I>(std::forward<S>(s), std::forward<A>(args)...));
  assert((SQLITE_DONE == r) || (SQLITE_ROW == r));

  return SQLITE_ROW == r ?
    std::optional<T>(get<T>(s, i)) :
    std::optional<T>();
}

template <typename T, int I = 1, typename S, typename ...A>
inline auto rexecget(S&& s, int const i = 0, A&& ...args) noexcept(
  noexcept(rexec<I>(std::forward<S>(s), std::forward<A>(args)...),
    get<T>(s, i)
  )
)
{
  auto const r(rexec<I>(std::forward<S>(s), std::forward<A>(args)...));
  assert((SQLITE_DONE == r) || (SQLITE_ROW == r));

  return SQLITE_ROW == r ?
    std::optional<T>(get<T>(s, i)) :
    std::optional<T>();
}

template <typename T, typename D, typename A, typename ...B,
  typename = std::enable_if_t<
    std::is_same_v<remove_cvr_t<D>, sqlite3*> ||
    is_db_v<D>
  >
>
inline auto execget(D&& db, A&& a, int const i = 0, B&& ...b) noexcept(
  noexcept(
    execget<T>(
      make_unique(std::forward<D>(db), std::forward<A>(a)),
      i,
      std::forward<B>(b)...
    )
  )
)
{
  return execget<T>(
    make_unique(std::forward<D>(db), std::forward<A>(a)),
    i,
    std::forward<B>(b)...
  );
}

namespace detail
{

struct maker
{
  std::string_view const s_;

  template <typename A, typename ...B>
  auto exec(A&& a, B&& ...b) && noexcept(
    noexcept(squ::exec(std::forward<A>(a), s_, std::forward<B>(b)...)))
  {
    return squ::exec(std::forward<A>(a), s_, std::forward<B>(b)...);
  }

  template <typename T, typename A, typename ...B>
  auto execget(A&& a, int const i = 0, B&& ...b) && noexcept(
    noexcept(squ::execget<T>(std::forward<A>(a), s_, i,
      std::forward<B>(b)...)))
  {
    return squ::execget<T>(std::forward<A>(a), s_, i,
      std::forward<B>(b)...);
  }

  template <typename A>
  auto execmulti(A&& a) && noexcept(
    noexcept(squ::execmulti(std::forward<A>(a), s_)))
  {
    return squ::execmulti(std::forward<A>(a), s_);
  }

  template <typename A>
  auto shared(A&& a, unsigned fl = 0) && noexcept(
    noexcept(squ::make_shared(std::forward<A>(a), s_, fl)))
  {
    return squ::make_shared(std::forward<A>(a), s_, fl);
  }

  template <typename A>
  auto unique(A&& a, unsigned fl = 0) && noexcept(
    noexcept(squ::make_unique(std::forward<A>(a), s_, fl)))
  {
    return squ::make_unique(std::forward<A>(a), s_, fl);
  }
};

}

namespace literals
{

inline auto operator "" _squ(char const* const s,
  std::size_t const N) noexcept
{
  return detail::maker{{s, N}};
}

}

//changes/////////////////////////////////////////////////////////////////////
inline auto changes(sqlite3* const db) noexcept
{
  return sqlite3_changes(db);
}

template <typename D, typename = std::enable_if_t<is_db_v<D>>>
inline auto changes(D const& db) noexcept(noexcept(changes(db.get())))
{
  return changes(db.get());
}

//clear_bindings//////////////////////////////////////////////////////////////
inline auto clear_bindings(sqlite3_stmt* const s) noexcept
{
  return sqlite3_clear_bindings(s);
}

template <typename S, typename = std::enable_if_t<is_stmt_v<S>>>
inline auto clear_bindings(S const& s) noexcept(
  noexcept(clear_bindings(s.get()))
)
{
  return clear_bindings(s.get());
}

//column_bytes////////////////////////////////////////////////////////////////
inline auto column_bytes(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return sqlite3_column_bytes(s, i);
}

template <typename S, typename = std::enable_if_t<is_stmt_v<S>>>
inline auto column_bytes(S const& s, int const i = 0) noexcept(
  noexcept(column_bytes(s.get(), i))
)
{
  return column_bytes(s.get(), i);
}

//column_count////////////////////////////////////////////////////////////////
inline auto column_count(sqlite3_stmt* const s) noexcept
{
  return sqlite3_column_count(s);
}

template <typename S, typename = std::enable_if_t<is_stmt_v<S>>>
inline auto column_count(S const& s) noexcept(noexcept(column_count(s.get())))
{
  return column_count(s.get());
}

//column_name/////////////////////////////////////////////////////////////////
inline auto column_name(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return sqlite3_column_name(s, i);
}

template <typename S, typename = std::enable_if_t<is_stmt_v<S>>>
inline auto column_name(S const& s, int const i = 0) noexcept(
  noexcept(column_name(s.get(), i))
)
{
  return column_name(s.get(), i);
}

//column_name16///////////////////////////////////////////////////////////////
inline auto column_name16(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return static_cast<char16_t const*>(sqlite3_column_name16(s, i));
}

template <typename S, typename = std::enable_if_t<is_stmt_v<S>>>
inline auto column_name16(S const& s, int const i = 0) noexcept(
  noexcept(column_name16(s.get(), i))
)
{
  return column_name16(s.get(), i);
}

//open_shared/////////////////////////////////////////////////////////////////
inline auto open_shared(char const* const filename, int const flags,
  char const* const zvfs = nullptr) noexcept
{
  sqlite3* db;

  return SQLITE_OK == sqlite3_open_v2(filename, &db, flags, zvfs) ?
    shared_db_t(db, detail::sqlite3_deleter()) :
    (detail::sqlite3_deleter()(db), shared_db_t());
}

template <typename ...A>
inline auto open_shared(std::string const& filename, A&& ...args) noexcept(
  noexcept(open_shared(filename.c_str(), std::forward<A>(args)...))
)
{
  return open_shared(filename.c_str(), std::forward<A>(args)...);
}

//open_unique/////////////////////////////////////////////////////////////////
inline auto open_unique(char const* const filename, int const flags,
  char const* const zvfs = nullptr) noexcept
{
  sqlite3* db;

  return SQLITE_OK == sqlite3_open_v2(filename, &db, flags, zvfs) ?
    unique_db_t(db) :
    (detail::sqlite3_deleter()(db), unique_db_t());
}

template <typename ...A>
inline auto open_unique(std::string const& filename, A&& ...args) noexcept(
  noexcept(open_unique(filename.c_str(), std::forward<A>(args)...))
)
{
  return open_unique(filename.c_str(), std::forward<A>(args)...);
}

//reset///////////////////////////////////////////////////////////////////////
inline auto reset(sqlite3_stmt* const s) noexcept
{
  return sqlite3_reset(s);
}

template <typename S, typename = std::enable_if_t<is_stmt_v<S>>>
inline auto reset(S const& s) noexcept(noexcept(reset(s.get())))
{
  return reset(s.get());
}

//for/////////////////////////////////////////////////////////////////////////
namespace detail
{

template <typename>
struct signature
{
};

//
template <typename>
struct remove_cv_seq;

//
template <typename R, typename ...A>
struct remove_cv_seq<R(A...)>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) volatile>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const volatile>
{
  using type = R(A...);
};

//
template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const noexcept>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) volatile noexcept>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const volatile noexcept>
{
  using type = R(A...);
};

//
template <typename R, typename ...A>
struct remove_cv_seq<R(A...) &>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const &>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) volatile &>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const volatile &>
{
  using type = R(A...);
};

//
template <typename R, typename ...A>
struct remove_cv_seq<R(A...) & noexcept>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const & noexcept >
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) volatile & noexcept>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const volatile & noexcept>
{
  using type = R(A...);
};

//
template <typename R, typename ...A>
struct remove_cv_seq<R(A...) &&>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const &&>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) volatile &&>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const volatile &&>
{
  using type = R(A...);
};

//
template <typename R, typename ...A>
struct remove_cv_seq<R(A...) && noexcept>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const && noexcept>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) volatile && noexcept>
{
  using type = R(A...);
};

template <typename R, typename ...A>
struct remove_cv_seq<R(A...) const volatile && noexcept>
{
  using type = R(A...);
};

template <typename F>
constexpr inline auto extract_signature(F* const) noexcept
{
  return signature<typename remove_cv_seq<F>::type>();
}

template <typename C, typename F>
constexpr inline auto extract_signature(F C::* const) noexcept
{
  return signature<typename remove_cv_seq<F>::type>();
}

template <typename F>
constexpr inline auto extract_signature(F const&) noexcept ->
  decltype(&F::operator(), extract_signature(&F::operator()))
{
  return extract_signature(&F::operator());
}

template <typename ...A, typename F, typename S, std::size_t ...I>
inline auto foreach_row(S&& s, F const f, int const i,
  signature<bool(std::size_t, A...)>, std::index_sequence<I...>) noexcept(
    noexcept(f(std::declval<remove_cvr_t<A>>()...))
  )
{
  decltype(exec(s)) r;

  for (std::size_t j{};; ++j)
  {
    switch (r = exec(std::forward<S>(s)))
    {
      case SQLITE_ROW:
        if (f(j, get<remove_cvr_t<A>>(
          std::forward<S>(s),
          i + detail::count_types_n<I, 0, A...>{})...)
        )
        {
          break;
        }
        else
        {
          continue;
        }

      case SQLITE_DONE:;
        break;

      default:
        assert(!"unhandled result from exec");
    }

    break;
  }

  return r;
}

template <typename ...A, typename F, typename S, std::size_t ...I>
inline auto foreach_row(S&& s, F const f, int const i,
  signature<bool(A...)>, std::index_sequence<I...>) noexcept(
    noexcept(f(std::declval<remove_cvr_t<A>>()...))
  )
{
  decltype(exec(s)) r;

  for (;;)
  {
    switch (r = exec(std::forward<S>(s)))
    {
      case SQLITE_ROW:
        if (f(get<remove_cvr_t<A>>(
          std::forward<S>(s),
          i + detail::count_types_n<I, 0, A...>{})...)
        )
        {
          break;
        }
        else
        {
          continue;
        }

      case SQLITE_DONE:;
        break;

      default:
        assert(!"unhandled result from exec");
    }

    break;
  }

  return r;
}

template <typename ...A, typename F, typename S, std::size_t ...I>
inline auto foreach_row(S&& s, F const f, int const i,
  signature<void(A...)>, std::index_sequence<I...>) noexcept(
    noexcept(f(std::declval<remove_cvr_t<A>>()...))
  )
{
  decltype(exec(s)) r;

  for (;;)
  {
    switch (r = exec(std::forward<S>(s)))
    {
      case SQLITE_ROW:
        f(get<remove_cvr_t<A>>(
          std::forward<S>(s),
          i + detail::count_types_n<I, 0, A...>{})...);
        continue;

      case SQLITE_DONE:;
        break;

      default:
        assert(!"unhandled result from exec");
    }

    break;
  }

  return r;
}

template <typename R, typename ...A, typename F, typename S, std::size_t ...I>
inline auto foreach_row(S&& s, F&& f, int const i,
  signature<R(A...)>) noexcept(
    noexcept(foreach_row(std::forward<S>(s),
        std::forward<F>(f),
        i,
        extract_signature(f),
        std::index_sequence_for<A...>()
      )
    )
  )
{
  return foreach_row(std::forward<S>(s),
    std::forward<F>(f),
    i,
    signature<R(A...)>{},
    std::index_sequence_for<A...>()
  );
}

}

template <typename F, typename S>
inline auto foreach_row(S&& s, F&& f, int const i = 0) noexcept(
    noexcept(
      detail::foreach_row(std::forward<S>(s),
        std::forward<F>(f),
        i,
        detail::extract_signature(f)
      )
    )
  )
{
  return detail::foreach_row(std::forward<S>(s),
    std::forward<F>(f),
    i,
    detail::extract_signature(f)
  );
}

template <typename F>
inline std::enable_if_t<std::is_invocable_r_v<void, F, sqlite3_stmt*>>
foreach_stmt(sqlite3* const db, F const f) noexcept(noexcept(f(nullptr)))
{
  for (auto s(sqlite3_next_stmt(db, {})); s; s = sqlite3_next_stmt(db, s))
  {
    f(s);
  }
}

template <typename F>
inline std::enable_if_t<std::is_invocable_r_v<bool, F, sqlite3_stmt*>>
foreach_stmt(sqlite3* const db, F const f) noexcept(noexcept(f(nullptr)))
{
  for (auto s(sqlite3_next_stmt(db, {}));
    s && !f(s);
    s = sqlite3_next_stmt(db, s));
}

template <typename D, typename ...A, typename = std::enable_if_t<is_db_v<D>>>
inline auto foreach_stmt(D const& db, A&& ...args) noexcept(
  noexcept(foreach_stmt(db.get(), std::forward<A>(args)...))
)
{
  return foreach_stmt(db.get(), std::forward<A>(args)...);
}

namespace detail
{

//container_push//////////////////////////////////////////////////////////////
template <typename FP, FP fp, typename C, typename S>
inline auto container_push(S&& s, C& c, int const i)
{
  decltype(exec(std::forward<S>(s))) r;

  for (;;)
  {
    switch (r = exec(std::forward<S>(s)))
    {
      case SQLITE_ROW:
        (c.*fp)(get<typename C::value_type>(std::forward<S>(s), i));

        continue;

      case SQLITE_DONE:
        break;

      default:
        assert(!"unhandled result from exec");
    }

    break;
  }

  return r;
}

template <typename FP, FP fp, typename C, typename S, typename T>
inline auto container_push(S&& s, C& c, T n, int const i)
{
  decltype(exec(std::forward<S>(s))) r(SQLITE_DONE);

  while (n--)
  {
    switch (r = exec(std::forward<S>(s)))
    {
      case SQLITE_ROW:
        (c.*fp)(get<typename C::value_type>(std::forward<S>(s), i));

        continue;

      case SQLITE_DONE:
        break;

      default:
        assert(!"unhandled result from exec");
    }

    break;
  }

  return r;
}

}

//emplace/////////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto emplace(S&& s, C& c, int const i = 0)
{
  return detail::container_push<
    decltype(&C::template emplace<typename C::value_type>),
    &C::template emplace<typename C::value_type>
  >(std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto emplace_n(S&& s, C& c, T&& n, int const i = 0)
{
  return detail::container_push<
    decltype(&C::template emplace<typename C::value_type>),
    &C::template emplace<typename C::value_type>
  >(std::forward<S>(s), c, std::forward<T>(n), i);
}

//emplace_back////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto emplace_back(S&& s, C& c, int const i = 0)
{
  return detail::container_push<
    decltype(&C::template emplace_back<typename C::value_type>),
    &C::template emplace_back<typename C::value_type>
  >(std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto emplace_back_n(S&& s, C& c, T&& n, int const i = 0)
{
  return detail::container_push<
    decltype(&C::template emplace_back<typename C::value_type>),
    &C::template emplace_back<typename C::value_type>
  >(std::forward<S>(s), c, std::forward<T>(n), i);
}

//insert//////////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto insert(S&& s, C& c, int const i = 0)
{
  return detail::container_push<
    decltype(&C::template insert<typename C::value_type>),
    &C::template insert<typename C::value_type>
  >(std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto insert_n(S&& s, C& c, T&& n, int const i = 0)
{
  return detail::container_push<
    decltype(&C::template insert<typename C::value_type>),
    &C::template insert<typename C::value_type>
  >(std::forward<S>(s), c, std::forward<T>(n), i);
}

//push_back///////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto push_back(S&& s, C& c, int const i = 0)
{
  return detail::container_push<
    decltype(&C::template push_back<typename C::value_type>),
    &C::template push_back<typename C::value_type>
  >(std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto push_back_n(S&& s, C& c, T&& n, int const i = 0)
{
  return detail::container_push<
    decltype(&C::template push_back<typename C::value_type>),
    &C::template push_back<typename C::value_type>
  >(std::forward<S>(s), c, std::forward<T>(n), i);
}

//reset_all///////////////////////////////////////////////////////////////////
inline void reset_all(sqlite3* const db) noexcept
{
  squ::foreach_stmt(db,
    [](auto const s) noexcept
    {
      squ::reset(s);
    }
  );
}

template <typename D, typename = std::enable_if_t<is_db_v<D>>>
inline auto reset_all(D const& db) noexcept(noexcept(reset_all(db.get())))
{
  return reset_all(db.get());
}

//reset_all_busy//////////////////////////////////////////////////////////////
inline void reset_all_busy(sqlite3* const db) noexcept
{
  squ::foreach_stmt(db,
    [](auto const s) noexcept
    {
      if (sqlite3_stmt_busy(s))
      {
        squ::reset(s);
      }
    }
  );
}

template <typename D, typename = std::enable_if_t<is_db_v<D>>>
inline auto reset_all_busy(D&& db) noexcept(
  noexcept(reset_all_busy(db.get()))
)
{
  return reset_all_busy(db.get());
}

}

#endif // SQLITEUTILS_HPP

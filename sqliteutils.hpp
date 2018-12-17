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

#include <string>

#include <string_view>

#include <type_traits>

#include <utility>

#include "sqlite3.h"

namespace squ
{

using blobpair_t = std::pair<void const* const, sqlite3_uint64 const>;

using charpair_t = std::pair<char const* const, sqlite3_uint64 const>;

using charpair16_t = std::pair<char16_t const* const, sqlite3_uint64 const>;

using nullpair_t = std::pair<std::nullptr_t const, sqlite3_uint64 const>;

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
inline void set(sqlite3_stmt* const s, blobpair_t const& v) noexcept
{
  sqlite3_bind_blob64(s, I, v.first, v.second, SQLITE_STATIC);
}

template <int I, typename T>
inline std::enable_if_t<
  std::is_floating_point<T>{},
  void
>
set(sqlite3_stmt* const s, T const v) noexcept
{
  sqlite3_bind_double(s, I, v);
}

template <int I, typename T>
inline std::enable_if_t<
  std::is_integral<T>{} && (sizeof(T) <= sizeof(int)),
  void
>
set(sqlite3_stmt* const s, T const v) noexcept
{
  sqlite3_bind_int(s, I, v);
}

template <int I, typename T>
inline std::enable_if_t<
  std::is_integral<T>{} && (sizeof(T) > sizeof(int)),
  void
>
set(sqlite3_stmt* const s, T const v) noexcept
{
  sqlite3_bind_int64(s, I, v);
}

template <int I>
inline void set(sqlite3_stmt* const s, std::nullptr_t const) noexcept
{
  sqlite3_bind_null(s, I);
}

template <int I, std::size_t N>
inline void set(sqlite3_stmt* const s, char const (&v)[N]) noexcept
{
  sqlite3_bind_text64(s, I, v, N, SQLITE_STATIC, SQLITE_UTF8);
}

template <int I>
inline void set(sqlite3_stmt* const s, charpair_t const& v) noexcept
{
  sqlite3_bind_text64(s, I, v.first, v.second, SQLITE_TRANSIENT, SQLITE_UTF8);
}

template <int I, typename T,
  typename = std::enable_if_t<std::is_same<T, char>{} >
>
inline void set(sqlite3_stmt* const s, T const* const& v) noexcept
{
  sqlite3_bind_text64(s, I, v, -1, SQLITE_TRANSIENT, SQLITE_UTF8);
}

template <int I>
inline void set(sqlite3_stmt* const s, std::string const& v) noexcept
{
  sqlite3_bind_text64(s, I, v.c_str(), v.size(), SQLITE_TRANSIENT,
    SQLITE_UTF8);
}

template <int I, std::size_t N>
inline void set(sqlite3_stmt* const s, char16_t const (&v)[N]) noexcept
{
  sqlite3_bind_text64(s, I, reinterpret_cast<char const*>(&*v), N,
    SQLITE_STATIC, SQLITE_UTF16);
}

template <int I>
inline void set(sqlite3_stmt* const s, char16_t const* v) noexcept
{
  sqlite3_bind_text64(s, I, reinterpret_cast<char const*>(v), -1,
    SQLITE_TRANSIENT, SQLITE_UTF16);
}

template <int I>
inline void set(sqlite3_stmt* const s, charpair16_t const& v) noexcept
{
  sqlite3_bind_text64(s, I, reinterpret_cast<char const*>(v.first),
    v.second, SQLITE_TRANSIENT, SQLITE_UTF16);
}

template <int I>
inline void set(sqlite3_stmt* const s, nullpair_t const& v) noexcept
{
  sqlite3_bind_zeroblob64(s, I, v.second);
}

template <int I, std::size_t ...Is, typename ...A>
void set(sqlite3_stmt* const s, std::index_sequence<Is...> const,
  A&& ...args) noexcept(noexcept((set<I + Is>(s, args), ...)))
{
  (set<I + Is>(s, args), ...);
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
    std::is_same<remove_cvr_t<T>, shared_db_t>{} ||
    std::is_same<remove_cvr_t<T>, unique_db_t>{}
  >;

using shared_stmt_t = std::shared_ptr<sqlite3_stmt>;

using unique_stmt_t = std::unique_ptr<sqlite3_stmt,
  detail::sqlite3_stmt_deleter
>;

template <typename T>
using is_stmt_t = 
  std::integral_constant<
    bool,
    std::is_same<remove_cvr_t<T>, shared_stmt_t>{} ||
    std::is_same<remove_cvr_t<T>, unique_stmt_t>{}
  >;

//set/////////////////////////////////////////////////////////////////////////
template <int I = 0, typename ...A>
inline void set(sqlite3_stmt* const s, A&& ...args) noexcept
{
  detail::set<I + 1>(s,
    std::make_index_sequence<sizeof...(A)>(),
    std::forward<A>(args)...
  );
}

template <int I = 0, typename S, typename ...A,
  typename = std::enable_if_t<is_stmt_t<S>{}>
>
inline auto set(S const& s, A&& ...args) noexcept(
  noexcept(set(s.get(), std::forward<A>(args)...))
)
{
  return set<I>(s.get(), std::forward<A>(args)...);
}

//rset////////////////////////////////////////////////////////////////////////
template <int I = 0, typename ...A>
inline void rset(sqlite3_stmt* const s, A&& ...args) noexcept
{
  sqlite3_reset(s);

  detail::set<I + 1>(s,
    std::make_index_sequence<sizeof...(A)>(),
    std::forward<A>(args)...
  );
}

template <int I = 0, typename S, typename ...A,
  typename = std::enable_if_t<is_stmt_t<S>{}>
>
inline auto rset(S const& s, A&& ...args) noexcept(
  noexcept(rset(s.get(), std::forward<A>(args)...))
)
{
  return rset<I>(s.get(), std::forward<A>(args)...);
}

//make_unique/////////////////////////////////////////////////////////////////
inline auto make_unique(sqlite3* const db,
  std::string_view const& sv) noexcept
{
  sqlite3_stmt* s;

#ifndef NDEBUG
  auto result(sqlite3_prepare_v3(db, sv.data(), sv.size(), 0, &s, nullptr));
  assert(SQLITE_OK == result);
#else
  sqlite3_prepare_v3(db, a, size, 0, &s, nullptr);
#endif // NDEBUG
  return unique_stmt_t(s);
}

template <std::size_t N>
inline auto make_unique(sqlite3* const db, char const (&a)[N]) noexcept
{
  sqlite3_stmt* s;

#ifndef NDEBUG
  auto const result(sqlite3_prepare_v3(db, a, N, 0, &s, nullptr));
  assert(SQLITE_OK == result);
#else
  sqlite3_prepare_v3(db, a, N, 0, &s, nullptr);
#endif // NDEBUG
  return unique_stmt_t(s);
}

// forwarders
template <typename D, typename ...A,
  typename = std::enable_if_t<is_db_t<D>{}>
>
inline auto make_unique(D const& db, A&& ...args) noexcept(
  noexcept(make_unique(db.get(), std::forward<A>(args)...))
)
{
  return make_unique(db.get(), std::forward<A>(args)...);
}

//make_shared/////////////////////////////////////////////////////////////////
inline auto make_shared(sqlite3* const db,
  std::string_view const& sv) noexcept
{
  sqlite3_stmt* s;

#ifndef NDEBUG
  auto result(sqlite3_prepare_v3(db, sv.data(), sv.size(), 0, &s, nullptr));
  assert(SQLITE_OK == result);
#else
  sqlite3_prepare_v3(db, a, size, 0, &s, nullptr);
#endif // NDEBUG
  return shared_stmt_t(s, detail::sqlite3_stmt_deleter());
}

template <std::size_t N>
inline auto make_shared(sqlite3* const db, char const (&a)[N]) noexcept
{
  sqlite3_stmt* s;

#ifndef NDEBUG
  auto const result(sqlite3_prepare_v3(db, a, N, 0, &s, nullptr));
  assert(SQLITE_OK == result);
#else
  sqlite3_prepare_v3(db, a, N, 0, &s, nullptr);
#endif // NDEBUG
  return shared_stmt_t(s, detail::sqlite3_stmt_deleter());
}

// forwarders
template <typename D, typename ...A,
  typename = std::enable_if_t<is_db_t<D>{}>
>
inline auto make_shared(D const& db, A&& ...args) noexcept(
  noexcept(make_shared(db.get(), std::forward<A>(args)...))
)
{
  return make_shared(db.get(), std::forward<A>(args)...);
}

//exec////////////////////////////////////////////////////////////////////////
template <int I = 0>
inline auto exec(sqlite3_stmt* const s) noexcept
{
  return sqlite3_step(s);
}

template <int I = 0, typename ...A>
inline auto exec(sqlite3_stmt* const s, A&& ...args) noexcept
{
  set<I>(s, std::forward<A>(args)...);

  return exec(s);
}

template <int I = 0>
inline auto rexec(sqlite3_stmt* const s) noexcept
{
  sqlite3_reset(s);

  return sqlite3_step(s);
}

template <int I = 0, typename ...A>
inline auto rexec(sqlite3_stmt* const s, A&& ...args) noexcept
{
  sqlite3_reset(s);

  return exec<I>(s, std::forward<A>(args)...);
}

// forwarders
template <int I = 0, typename S, typename ...A,
  typename = std::enable_if_t<is_stmt_t<S>{}>
>
inline auto exec(S const& s, A&& ...args) noexcept(
  noexcept(exec(s.get(), std::forward<A>(args)...))
)
{
  return exec<I>(s.get(), std::forward<A>(args)...);
}

template <int I = 0, typename S, typename ...A,
  typename = std::enable_if_t<is_stmt_t<S>{}>
>
inline auto rexec(S const& s, A&& ...args) noexcept(
  noexcept(rexec(s.get(), std::forward<A>(args)...))
)
{
  return rexec<I>(s.get(), std::forward<A>(args)...);
}

template <int I = 0, typename A, typename ...B>
inline auto exec(sqlite3* const db, A&& a, B&& ...args) noexcept(
  noexcept(
    exec<I>(
      make_unique(db, std::forward<A>(a)), std::forward<B>(args)...
    )
  )
)
{
  return exec<I>(
    make_unique(db, std::forward<A>(a)), std::forward<B>(args)...
  );
}

template <typename D, typename ...A,
  typename = std::enable_if_t<is_db_t<D>{}>
>
inline auto exec(D const& db, A&& ...args) noexcept(
  noexcept(exec(db.get(), std::forward<A>(args)...))
)
{
  return exec(db.get(), std::forward<A>(args)...);
}

//exec_multi//////////////////////////////////////////////////////////////////
template <typename T>
inline std::enable_if_t<
  std::is_same<char const*, std::decay_t<T> >{},
  decltype(sqlite3_exec(std::declval<sqlite3*>(), std::declval<T>(),
    nullptr, nullptr, nullptr)
  )
>
exec_multi(sqlite3* const db, T&& a) noexcept
{
  return sqlite3_exec(db, a, nullptr, nullptr, nullptr);
}

template <typename T>
inline std::enable_if_t<
  std::is_same<std::string, std::decay_t<T> >{},
  decltype(exec_multi(std::declval<sqlite3*>(), std::declval<T>()))
>
exec_multi(sqlite3* const db, T&& a) noexcept
{
  return exec_multi(db, a.c_str());
}

template <typename D, typename ...A,
  typename = std::enable_if_t<is_db_t<D>{}>
>
inline auto exec_multi(D const& db, A&& ...args) noexcept(
  noexcept(exec_multi(db.get(), std::forward<A>(args)...))
)
{
  return exec_multi(db.get(), std::forward<A>(args)...);
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
  static_assert(!std::is_reference<T>{});
  return sqlite3_column_int(s, i);
}

template <typename T>
inline std::enable_if_t<
  std::is_integral<T>{} &&
  (sizeof(T) > sizeof(std::int32_t)),
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  static_assert(!std::is_reference<T>{});
  return sqlite3_column_int64(s, i);
}

template <typename T>
inline std::enable_if_t<
  std::is_floating_point<T>{},
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  static_assert(!std::is_reference<T>{});
  return sqlite3_column_double(s, i);
}

template <typename T>
inline std::enable_if_t<
  std::is_same<T, char const*>{},
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  static_assert(!std::is_reference<T>{});
  return reinterpret_cast<char const*>(sqlite3_column_text(s, i));
}

template <typename T>
inline std::enable_if_t<
  std::is_same<T, charpair_t>{},
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  static_assert(!std::is_reference<T>{});
  return {
    get<char const*>(s, i),
    sqlite3_column_bytes(s, i)
  };
}

template <typename T>
inline std::enable_if_t<
  std::is_same<T, std::string>{},
  T
>
get(sqlite3_stmt* const s, int const i = 0)
{
  static_assert(!std::is_reference<T>{});
  return {
    get<char const*>(s, i),
    std::string::size_type(sqlite3_column_bytes(s, i))
  };
}

template <typename T>
inline std::enable_if_t<
  std::is_same<T, void const*>{},
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  static_assert(!std::is_reference<T>{});
  return sqlite3_column_blob(s, i);
}

template <typename T>
inline std::enable_if_t<
  std::is_same<T, blobpair_t>{},
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept
{
  static_assert(!std::is_reference<T>{});
  return {
    get<void const*>(s, i),
    sqlite3_column_bytes(s, i)
  };
}

namespace detail
{

template <typename>
struct is_std_pair : std::false_type { };

template <class T1, class T2>
struct is_std_pair<std::pair<T1, T2> > : std::true_type { };

template <typename>
struct is_std_tuple : std::false_type { };

template <class ...A>
struct is_std_tuple<std::tuple<A...> > : std::true_type { };

template <typename A, typename ...B>
struct count_types :
  std::integral_constant<int, count_types<A>{} + count_types<B...>{}>
{
};

template <typename A>
struct count_types<A> : std::integral_constant<int, 1>
{
};

template <>
struct count_types<blobpair_t> :
  std::integral_constant<int, 1>
{
};

template <>
struct count_types<charpair_t> :
  std::integral_constant<int, 1>
{
};

template <>
struct count_types<charpair16_t> :
  std::integral_constant<int, 1>
{
};

template <>
struct count_types<nullpair_t> :
  std::integral_constant<int, 1>
{
};

template <typename A, typename B>
struct count_types<std::pair<A, B> > :
  std::integral_constant<int, count_types<A>{} + count_types<B>{}>
{
};

template <typename A, typename ...B>
struct count_types<std::tuple<A, B...> > :
  std::integral_constant<int, count_types<A>{} + count_types<B...>{}>
{
};

template <std::size_t I, int S, typename A, typename ...B>
struct count_types_n : count_types_n<I - 1, S + count_types<A>{}, B...>
{
};

template <int S, typename A, typename ...B>
struct count_types_n<0, S, A, B...> : std::integral_constant<int, S>
{
};

template <typename T, std::size_t ...I>
T make_tuple(sqlite3_stmt* const s, int const i,
  std::index_sequence<I...>)
{
  return T{
    get<std::tuple_element_t<I, T>>(s,
      i + count_types_n<I, 0, typename std::tuple_element_t<I, T>...>{}
    )...
  };
}

}

template <typename T>
inline std::enable_if_t<
  (detail::is_std_pair<T>{} ||
  detail::is_std_tuple<T>{}) &&
  !std::is_same<T, blobpair_t>{} &&
  !std::is_same<T, charpair_t>{} &&
  !std::is_same<T, charpair16_t>{} &&
  !std::is_same<T, nullpair_t>{},
  T
>
get(sqlite3_stmt* const s, int const i = 0) noexcept(
  noexcept(
    detail::make_tuple<T>(s,
      i,
      std::make_index_sequence<std::size_t(std::tuple_size<T>{})>()
    )
  )
)
{
  static_assert(!std::is_reference<T>{});
  return detail::make_tuple<T>(s,
    i,
    std::make_index_sequence<std::tuple_size<T>{}>()
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
  typename = typename std::enable_if_t<is_stmt_t<S>{}>
>
inline auto get(S const& s, int const i = 0) noexcept(
  noexcept(get<A...>(s.get(), i))
)
{
  return get<A...>(s.get(), i);
}

//execget/////////////////////////////////////////////////////////////////////
template <typename T, int I = 0, typename S, typename ...A>
inline auto execget(S&& s, int const i = 0, A&& ...args) noexcept(
  noexcept(exec<I>(std::forward<S>(s), std::forward<A>(args)...),
    get<T>(s, i)
  )
)
{
#ifndef NDEBUG
  auto const r(exec<I>(std::forward<S>(s), std::forward<A>(args)...));
  assert(SQLITE_ROW == r);
#else
  exec<I>(std::forward<S>(s), std::forward<A>(args)...);
#endif // NDEBUG

  return get<T>(s, i);
}

template <typename T, int I = 0, typename S, typename ...A>
inline auto rexecget(S&& s, int const i = 0, A&& ...args) noexcept(
  noexcept(rexec<I>(std::forward<S>(s), std::forward<A>(args)...),
    get<T>(s, i)
  )
)
{
#ifndef NDEBUG
  auto const r(rexec<I>(std::forward<S>(s), std::forward<A>(args)...));
  assert(SQLITE_ROW == r);
#else
  rexec<I>(std::forward<S>(s), std::forward<A>(args)...);
#endif // NDEBUG

  return get<T>(s, i);
}

template <typename T, int I = 0, typename D, typename A, typename ...B,
  typename = std::enable_if_t<
    std::is_same<remove_cvr_t<D>, sqlite3*>{} ||
    std::is_same<remove_cvr_t<D>, shared_db_t>{} ||
    std::is_same<remove_cvr_t<D>, unique_db_t>{}
  >
>
inline auto execget(D&& db, A&& a, int const i = 0, B&& ...args) noexcept(
  noexcept(
    execget<T, I>(
      make_unique(std::forward<D>(db), std::forward<A>(a)),
      i,
      std::forward<B>(args)...
    )
  )
)
{
  return execget<T, I>(
    make_unique(std::forward<D>(db), std::forward<A>(a)),
    i,
    std::forward<B>(args)...
  );
}

//col/////////////////////////////////////////////////////////////////////////
class col
{
  int const i_;

  sqlite3_stmt* stmt_;

public:
  explicit col(decltype(i_) const i) noexcept : i_(i) { }

  col(col const&) = delete;

  col& operator=(col const&) = delete;

  void set_stmt(sqlite3_stmt* const s) && noexcept { stmt_ = s; }

  operator int() const && noexcept
  {
    return sqlite3_column_int(stmt_, i_);
  }

  operator sqlite3_int64() const && noexcept
  {
    return sqlite3_column_int64(stmt_, i_);
  }

  operator double() const && noexcept
  {
    return sqlite3_column_double(stmt_, i_);
  }

  operator char const*() const && noexcept
  {
    return reinterpret_cast<char const*>(sqlite3_column_text(stmt_, i_));
  }

  operator std::string() const &&
  {
    return {
      reinterpret_cast<char const*>(sqlite3_column_text(stmt_, i_)),
      std::string::size_type(sqlite3_column_bytes(stmt_, i_))
    };
  }

  operator void const*() const && noexcept
  {
    return sqlite3_column_blob(stmt_, i_);
  }

  operator blobpair_t() const && noexcept
  {
    return {
      sqlite3_column_blob(stmt_, i_),
      sqlite3_column_bytes(stmt_, i_)
    };
  }

  operator charpair_t() const && noexcept
  {
    return {
      reinterpret_cast<char const*>(sqlite3_column_text(stmt_, i_)),
      sqlite3_column_bytes(stmt_, i_)
    };
  }

  operator charpair16_t() const && noexcept
  {
    return {
      static_cast<char16_t const*>(sqlite3_column_text16(stmt_, i_)),
      sqlite3_column_bytes(stmt_, i_)
    };
  }
};

template <typename S,
  typename = std::enable_if_t<is_stmt_t<S>{}>
>
inline decltype(auto) operator|(S const& s, col&& c) noexcept
{
  assert(s);

  std::move(c).set_stmt(s.get());

  return c;
}

//changes/////////////////////////////////////////////////////////////////////
inline auto changes(sqlite3* const db) noexcept
{
  return sqlite3_changes(db);
}

template <typename D,
  typename = std::enable_if_t<is_db_t<D>{}>
>
inline auto changes(D const& db) noexcept(noexcept(changes(db.get())))
{
  return changes(db.get());
}

//clear_bindings//////////////////////////////////////////////////////////////
inline auto clear_bindings(sqlite3_stmt* const s) noexcept
{
  return sqlite3_clear_bindings(s);
}

template <typename S, typename = std::enable_if_t<is_stmt_t<S>{}> >
inline auto clear_bindings(S const& s) noexcept(
  noexcept(clear_bindings(s.get()))
)
{
  return clear_bindings(s.get());
}

//column_count////////////////////////////////////////////////////////////////
inline auto column_count(sqlite3_stmt* const s) noexcept
{
  return sqlite3_column_count(s);
}

template <typename S, typename = std::enable_if_t<is_stmt_t<S>{}> >
inline auto column_count(S const& s) noexcept(
  noexcept(column_count(s.get()))
)
{
  return column_count(s.get());
}

//column_name/////////////////////////////////////////////////////////////////
inline auto column_name(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return sqlite3_column_name(s, i);
}

template <typename S, typename = std::enable_if_t<is_stmt_t<S>{}> >
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

template <typename S, typename = std::enable_if_t<is_stmt_t<S>{}> >
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

  if (SQLITE_OK == sqlite3_open_v2(filename, &db, flags, zvfs))
  {
    return shared_db_t(db, detail::sqlite3_deleter());
  }
  else
  {
    detail::sqlite3_deleter()(db);

    return shared_db_t();
  }
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

  if (SQLITE_OK == sqlite3_open_v2(filename, &db, flags, zvfs))
  {
    return unique_db_t(db);
  }
  else
  {
    detail::sqlite3_deleter()(db);

    return unique_db_t();
  }
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

template <typename S, typename = std::enable_if_t<is_stmt_t<S>{}> >
inline auto reset(S const& s) noexcept(noexcept(reset(s.get())))
{
  return reset(s.get());
}

//size////////////////////////////////////////////////////////////////////////
inline auto size(sqlite3_stmt* const s, int const i = 0) noexcept
{
  return sqlite3_column_bytes(s, i);
}

template <typename S, typename = std::enable_if_t<is_stmt_t<S>{}> >
inline auto size(S const& s, int const i = 0) noexcept(
  noexcept(size(s.get(), i))
)
{
  return size(s.get(), i);
}

template <typename ...A>
inline auto bytes(A&& ...args) noexcept(
  noexcept(size(std::forward<A>(args)...))
)
{
  return size(std::forward<A>(args)...);
}

//for/////////////////////////////////////////////////////////////////////////
namespace
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

template <typename R, typename ...A, typename F, typename S, std::size_t ...Is>
inline auto foreach_row(S&& s, F const f, int const i,
  signature<R(A...)> const, std::index_sequence<Is...> const) noexcept(
    noexcept(f(std::declval<A>()...))
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
          i + detail::count_types_n<Is, 0, A...>{})...)
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

template <typename R, typename ...A, typename F, typename S, std::size_t ...Is>
inline auto foreach_row(S&& s, F&& f, int const i,
  signature<R(A...)> const) noexcept(
    noexcept(foreach_row(std::forward<S>(s),
        std::forward<F>(f),
        i,
        extract_signature(f),
        std::make_index_sequence<sizeof...(A)>()
      )
    )
  )
{
  return foreach_row(std::forward<S>(s),
    std::forward<F>(f),
    i,
    extract_signature(f),
    std::make_index_sequence<sizeof...(A)>()
  );
}

}

template <typename F, typename S>
inline auto foreach_row(S&& s, F&& f, int const i = 0) noexcept(
    noexcept(
      foreach_row(std::forward<S>(s),
        std::forward<F>(f),
        i,
        extract_signature(f)
      )
    )
  )
{
  return foreach_row(std::forward<S>(s),
    std::forward<F>(f),
    i,
    extract_signature(f)
  );
}

template <typename F>
inline void foreach_stmt(sqlite3* const db, F const f) noexcept(
  noexcept(f(std::declval<sqlite3_stmt*>()))
)
{
  for (auto s(sqlite3_next_stmt(db, {}));
    s && f(s);
    s = sqlite3_next_stmt(db, s));
}

template <typename D, typename ...A,
  typename = std::enable_if_t<is_db_t<D>{}>
>
inline auto foreach_stmt(D const& db, A&& ...args) noexcept(
  noexcept(foreach_stmt(db.get(), std::forward<A>(args)...))
)
{
  return foreach_stmt(db.get(), std::forward<A>(args)...);
}

template <typename F, typename S>
inline auto foreach_stmt(S&& s, F const f) noexcept(noexcept(f()))
{
  decltype(exec(std::forward<S>(s))) r;

  for (;;)
  {
    switch (r = exec(std::forward<S>(s)))
    {
      case SQLITE_ROW:
        if (f())
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

namespace
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
inline auto container_push(S&& s, C& c, T const n, int const i)
{
  decltype(exec(std::forward<S>(s))) r(SQLITE_DONE);

  for (T j{}; j != n; ++j)
  {
    switch (r = exec(std::forward<S>(s)))
    {
      case SQLITE_ROW:
        (c.*fp)(get<typename C::value_type>(std::forward<S>(s), i));

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

}

//emplace/////////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto emplace(S&& s, C& c, int const i = 0)
{
  return container_push<
    decltype(&C::template emplace<typename C::value_type>),
    &C::template emplace<typename C::value_type>
  >(std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto emplace_n(S&& s, C& c, T&& n, int const i = 0)
{
  return container_push<
    decltype(&C::template emplace<typename C::value_type>),
    &C::template emplace<typename C::value_type>
  >(std::forward<S>(s), c, std::forward<T>(n), i);
}

//emplace_back////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto emplace_back(S&& s, C& c, int const i = 0)
{
  return container_push<
    decltype(&C::template emplace_back<typename C::value_type>),
    &C::template emplace_back<typename C::value_type>
  >(std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto emplace_back_n(S&& s, C& c, T&& n, int const i = 0)
{
  return container_push<
    decltype(&C::template emplace_back<typename C::value_type>),
    &C::template emplace_back<typename C::value_type>
  >(std::forward<S>(s), c, std::forward<T>(n), i);
}

//insert//////////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto insert(S&& s, C& c, int const i = 0)
{
  return container_push<
    decltype(&C::template insert<typename C::value_type>),
    &C::template insert<typename C::value_type>
  >(std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto insert_n(S&& s, C& c, T&& n, int const i = 0)
{
  return container_push<
    decltype(&C::template insert<typename C::value_type>),
    &C::template insert<typename C::value_type>
  >(std::forward<S>(s), c, std::forward<T>(n), i);
}

//push_back///////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto push_back(S&& s, C& c, int const i = 0)
{
  return container_push<
    decltype(&C::template push_back<typename C::value_type>),
    &C::template push_back<typename C::value_type>
  >(std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto push_back_n(S&& s, C& c, T&& n, int const i = 0)
{
  return container_push<
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

      return true;
    }
  );
}

template <typename D,
  typename = std::enable_if_t<is_db_t<D>{}>
>
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
      // else do nothing

      return true;
    }
  );
}

template <typename D,
  typename = std::enable_if_t<is_db_t<D>{}>
>
inline auto reset_all_busy(D const& db) noexcept(
  noexcept(reset_all_busy(db.get()))
)
{
  return reset_all_busy(db.get());
}

}

#endif // SQLITEUTILS_HPP

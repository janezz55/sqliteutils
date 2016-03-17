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

#if __cplusplus < 201402L
# error "You need a c++14 compiler"
#endif // __cplusplus

#include "sqlite3.h"

#include <cassert>

#include <memory>

#include <string>

#include <type_traits>

#include <utility>

namespace sqlite
{

using blobpair_t = ::std::pair<void const* const, sqlite3_uint64 const>;

using charpair_t = ::std::pair<char const* const, sqlite3_uint64 const>;

using char16pair_t = ::std::pair<char16_t const* const, sqlite3_uint64 const>;

using nullpair_t = ::std::pair<::std::nullptr_t const, sqlite3_uint64 const>;

namespace
{

struct swallow
{
  template <typename ...T>
  constexpr explicit swallow(T&& ...) noexcept
  {
  }
};

}

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
inline void set(sqlite3_stmt* const stmt, blobpair_t const& v) noexcept
{
  sqlite3_bind_blob64(stmt, I, v.first, v.second, SQLITE_STATIC);
}

template <int I, typename T>
inline typename ::std::enable_if<
  ::std::is_floating_point<T>{},
  void
>::type
set(sqlite3_stmt* const stmt, T const v) noexcept
{
  sqlite3_bind_double(stmt, I, v);
}

template <int I, typename T>
inline typename ::std::enable_if<
  ::std::is_integral<T>{} && (sizeof(T) <= sizeof(int)),
  void
>::type
set(sqlite3_stmt* const stmt, T const v) noexcept
{
  sqlite3_bind_int(stmt, I, v);
}

template <int I, typename T>
inline typename ::std::enable_if<
  ::std::is_integral<T>{} && (sizeof(T) > sizeof(int)),
  void
>::type
set(sqlite3_stmt* const stmt, T const v) noexcept
{
  sqlite3_bind_int64(stmt, I, v);
}

template <int I>
inline void set(sqlite3_stmt* const stmt, ::std::nullptr_t const) noexcept
{
  sqlite3_bind_null(stmt, I);
}

template <int I, ::std::size_t N>
inline void set(sqlite3_stmt* const stmt, char const (&v)[N]) noexcept
{
  sqlite3_bind_text64(stmt, I, v, N, SQLITE_STATIC, SQLITE_UTF8);
}

template <int I>
inline void set(sqlite3_stmt* const stmt, charpair_t const& v) noexcept
{
  sqlite3_bind_text64(stmt, I, v.first, v.second, SQLITE_TRANSIENT,
    SQLITE_UTF8);
}

template <int I, typename T,
  typename = typename std::enable_if<
    std::is_same<T, char>{}
  >::type
>
inline void set(sqlite3_stmt* const stmt, T const* const& v) noexcept
{
  sqlite3_bind_text64(stmt, I, v, -1, SQLITE_TRANSIENT, SQLITE_UTF8);
}

template <int I>
inline void set(sqlite3_stmt* const stmt, ::std::string const& v) noexcept
{
  sqlite3_bind_text64(stmt, I, v.c_str(), v.size(), SQLITE_TRANSIENT,
    SQLITE_UTF8);
}

template <int I, ::std::size_t N>
inline void set(sqlite3_stmt* const stmt, char16_t const (&v)[N]) noexcept
{
  sqlite3_bind_text64(stmt, I, reinterpret_cast<char const*>(v), N,
    SQLITE_STATIC, SQLITE_UTF16);
}

template <int I>
inline void set(sqlite3_stmt* const stmt, char16_t const* v) noexcept
{
  sqlite3_bind_text64(stmt, I, reinterpret_cast<char const*>(v), -1,
    SQLITE_TRANSIENT, SQLITE_UTF16);
}

template <int I>
inline void set(sqlite3_stmt* const stmt, char16pair_t const& v) noexcept
{
  sqlite3_bind_text64(stmt, I, reinterpret_cast<char const*>(v.first),
    v.second, SQLITE_TRANSIENT, SQLITE_UTF16);
}

template <int I>
inline void set(sqlite3_stmt* const stmt, nullpair_t const& v) noexcept
{
  sqlite3_bind_zeroblob64(stmt, I, v.second);
}

template <int I, ::std::size_t ...Is, typename ...A>
void set(sqlite3_stmt* const stmt, ::std::index_sequence<Is...> const,
  A&& ...args)
{
  swallow{(set<I + Is>(stmt, args), 0)...};
}

}

using shared_db_t = ::std::shared_ptr<sqlite3>;

using unique_db_t = ::std::unique_ptr<sqlite3, detail::sqlite3_deleter>;

template <typename T>
using remove_cvr_t = ::std::remove_cv_t<::std::remove_reference_t<T> >;

template <typename T>
using is_db_t = 
  ::std::integral_constant<
    bool,
    ::std::is_same<remove_cvr_t<T>, shared_db_t>{} ||
    ::std::is_same<remove_cvr_t<T>, unique_db_t>{}
  >;

using shared_stmt_t = ::std::shared_ptr<sqlite3_stmt>;

using unique_stmt_t = ::std::unique_ptr<sqlite3_stmt,
  detail::sqlite3_stmt_deleter
>;

template <typename T>
using is_stmt_t = 
  ::std::integral_constant<
    bool,
    ::std::is_same<remove_cvr_t<T>, shared_stmt_t>{} ||
    ::std::is_same<remove_cvr_t<T>, unique_stmt_t>{}
  >;

//set/////////////////////////////////////////////////////////////////////////
template <int I = 1, typename ...A>
inline void set(sqlite3_stmt* const stmt, A&& ...args) noexcept
{
  detail::set<I>(stmt,
    ::std::make_index_sequence<sizeof...(A)>(),
    ::std::forward<A>(args)...
  );
}

//make_unique/////////////////////////////////////////////////////////////////
template <typename T,
  typename = typename std::enable_if<
    std::is_same<T, char>{}
  >::type
>
inline auto make_unique(sqlite3* const db, T const* const& a,
  int const size = -1) noexcept
{
  sqlite3_stmt* stmt;

#ifndef NDEBUG
  auto const result(sqlite3_prepare_v2(db, a, size, &stmt, nullptr));
  assert(SQLITE_OK == result);
#else
  sqlite3_prepare_v2(db, a, size, &stmt, nullptr);
#endif // NDEBUG
  return unique_stmt_t(stmt);
}

template <::std::size_t N>
inline auto make_unique(sqlite3* const db, char const (&a)[N]) noexcept
{
  sqlite3_stmt* stmt;

#ifndef NDEBUG
  auto const result(sqlite3_prepare_v2(db, a, N, &stmt, nullptr));
  assert(SQLITE_OK == result);
#else
  sqlite3_prepare_v2(db, a, N, &stmt, nullptr);
#endif // NDEBUG
  return unique_stmt_t(stmt);
}

inline auto make_unique(sqlite3* const db, ::std::string const& a) noexcept
{
  return make_unique(db, a.c_str(), a.size());
}

// forwarders
template <typename D, typename ...A,
  typename = typename ::std::enable_if<is_db_t<D>{}>::type
>
inline auto make_unique(D const& db, A&& ...args) noexcept(
  noexcept(make_unique(db.get(), ::std::forward<A>(args)...))
)
{
  return make_unique(db.get(), ::std::forward<A>(args)...);
}

//make_shared/////////////////////////////////////////////////////////////////
template <typename T,
  typename = typename std::enable_if<
    std::is_same<T, char>{}
  >::type
>
inline auto make_shared(sqlite3* const db, T const* const& a,
  int const size = -1) noexcept
{
  sqlite3_stmt* stmt;

#ifndef NDEBUG
  auto const result(sqlite3_prepare_v2(db, a, size, &stmt, nullptr));
  assert(SQLITE_OK == result);
#else
  sqlite3_prepare_v2(db, a, size, &stmt, nullptr);
#endif // NDEBUG
  return shared_stmt_t(stmt, detail::sqlite3_stmt_deleter());
}

template <::std::size_t N>
inline auto make_shared(sqlite3* const db, char const (&a)[N]) noexcept
{
  sqlite3_stmt* stmt;

#ifndef NDEBUG
  auto const result(sqlite3_prepare_v2(db, a, N, &stmt, nullptr));
  assert(SQLITE_OK == result);
#else
  sqlite3_prepare_v2(db, a, N, &stmt, nullptr);
#endif // NDEBUG
  return shared_stmt_t(stmt, detail::sqlite3_stmt_deleter());
}

inline auto make_shared(sqlite3* const db, ::std::string const& a) noexcept
{
  return make_shared(db, a.c_str(), a.size());
}

// forwarders
template <typename D, typename ...A,
  typename = typename ::std::enable_if<is_db_t<D>{}>::type
>
inline auto make_shared(D const& db, A&& ...args) noexcept(
  noexcept(make_shared(db.get(), ::std::forward<A>(args)...))
)
{
  return make_shared(db.get(), ::std::forward<A>(args)...);
}

//exec////////////////////////////////////////////////////////////////////////
template <int I = 1>
inline auto exec(sqlite3_stmt* const stmt) noexcept
{
  return sqlite3_step(stmt);
}

template <int I = 1, typename ...A>
inline auto exec(sqlite3_stmt* const stmt, A&& ...args) noexcept
{
  set<I>(stmt, ::std::forward<A>(args)...);

  return exec(stmt);
}

template <int I = 1>
inline auto rexec(sqlite3_stmt* const stmt) noexcept
{
  sqlite3_reset(stmt);

  return sqlite3_step(stmt);
}

template <int I = 1, typename ...A>
inline auto rexec(sqlite3_stmt* const stmt, A&& ...args) noexcept
{
  sqlite3_reset(stmt);

  return exec<I>(stmt, ::std::forward<A>(args)...);
}

// forwarders
template <int I = 1, typename S, typename ...A,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline auto exec(S const& stmt, A&& ...args) noexcept(
  noexcept(exec(stmt.get(), ::std::forward<A>(args)...))
)
{
  return exec<I>(stmt.get(), ::std::forward<A>(args)...);
}

template <int I = 1, typename S, typename ...A,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline auto rexec(S const& stmt, A&& ...args) noexcept(
  noexcept(rexec(stmt.get(), ::std::forward<A>(args)...))
)
{
  return rexec<I>(stmt.get(), ::std::forward<A>(args)...);
}

template <int I = 1, typename A, typename ...B>
inline auto exec(sqlite3* const db, A&& a, B&& ...args) noexcept(
  noexcept(
    exec<I>(
      make_unique(db, ::std::forward<A>(a)),
      ::std::forward<B>(args)...
    )
  )
)
{
  return exec<I>(
    make_unique(db, ::std::forward<A>(a)),
    ::std::forward<B>(args)...
  );
}

inline auto exec(sqlite3* const db, char const* const a) noexcept
{
  return sqlite3_exec(db, a, nullptr, nullptr, nullptr);
}

inline auto exec(sqlite3* const db, ::std::string const& a) noexcept
{
  return exec(db, a.c_str());
}

// forwarders
template <typename D, typename ...A,
  typename = typename ::std::enable_if<is_db_t<D>{}>::type
>
inline auto exec(D const& db, A&& ...args) noexcept(
  noexcept(exec(db.get(), ::std::forward<A>(args)...))
)
{
  return exec(db.get(), ::std::forward<A>(args)...);
}

//get/////////////////////////////////////////////////////////////////////////
template <typename T>
inline typename ::std::enable_if<
  ::std::is_integral<T>{} && (sizeof(T) <= sizeof(int)),
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return sqlite3_column_int(stmt, i);
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_integral<T>{} && (sizeof(T) > sizeof(int)),
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return sqlite3_column_int64(stmt, i);
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_floating_point<T>{},
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return sqlite3_column_double(stmt, i);
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_same<T, char const*>{},
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return reinterpret_cast<char const*>(sqlite3_column_text(stmt, i));
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_same<T, charpair_t>{},
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return {
    get<char const*>(stmt, i),
    sqlite3_column_bytes(stmt, i)
  };
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_same<T, ::std::string>{},
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0)
{
  return {
    get<char const*>(stmt, i),
    ::std::string::size_type(sqlite3_column_bytes(stmt, i))
  };
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_same<T, void const*>{},
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return sqlite3_column_blob(stmt, i);
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_same<T, blobpair_t>{},
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return {
    get<void const*>(stmt, i),
    sqlite3_column_bytes(stmt, i)
  };
}

namespace
{

template <typename>
struct is_std_pair : ::std::false_type { };

template <class T1, class T2>
struct is_std_pair<::std::pair<T1, T2> > : ::std::true_type { };

template <typename>
struct is_std_tuple : ::std::false_type { };

template <class ...A>
struct is_std_tuple<::std::tuple<A...> > : ::std::true_type { };

template <typename A, typename ...B>
struct count_types :
  ::std::integral_constant<int, count_types<A>{} + count_types<B...>{}>
{
};

template <typename A>
struct count_types<A> : ::std::integral_constant<int, 1>
{
};

template <typename A, typename B>
struct count_types<::std::pair<A, B> > :
  ::std::integral_constant<int, count_types<A>{} + count_types<B>{}>
{
};

template <typename A, typename ...B>
struct count_types<::std::tuple<A, B...> > :
  ::std::integral_constant<int, count_types<A>{} + count_types<B...>{}>
{
};

template <::std::size_t I, int S, typename A, typename ...B>
struct count_types_n : count_types_n<I - 1, S + count_types<A>{}, B...>
{
};

template <int S, typename A, typename ...B>
struct count_types_n<0, S, A, B...> : ::std::integral_constant<int, S>
{
};

template <typename T, ::std::size_t ...Is>
T make_tuple(sqlite3_stmt* const stmt, int const i,
  ::std::index_sequence<Is...> const)
{
  return T{
    get<typename ::std::tuple_element<Is, T>::type>(stmt,
      i +
      count_types_n<Is, 0, typename ::std::tuple_element<Is, T>::type...>{}
    )...
  };
}

}

template <typename T>
inline typename ::std::enable_if<
  is_std_pair<T>{} ||
  is_std_tuple<T>{},
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept(
  noexcept(
    make_tuple<T>(stmt, i,
      ::std::make_index_sequence<::std::tuple_size<T>{}>()
    )
  )
)
{
  return make_tuple<T>(stmt, i,
    ::std::make_index_sequence<::std::tuple_size<T>{}>());
}

template <typename ...A,
  typename = typename ::std::enable_if<bool(sizeof...(A) > 1)>::type
>
::std::tuple<A...> get(sqlite3_stmt* const stmt, int i = 0) noexcept(
  noexcept(::std::tuple<A...>{get<A>(stmt, i++)...})
)
{
  return ::std::tuple<A...>{get<A>(stmt, i++)...};
}

template <typename T, typename S,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline auto get(S const& stmt, int const i = 0) noexcept(
  noexcept(get<T>(stmt.get(), i))
)
{
  return get<T>(stmt.get(), i);
}

//execget/////////////////////////////////////////////////////////////////////
template <typename T, int I = 1, typename S, typename ...A>
inline auto execget(S&& stmt, int const i = 0, A&& ...args) noexcept(
  noexcept(exec<I>(::std::forward<S>(stmt), ::std::forward<A>(args)...),
    get<T>(stmt, i)
  )
)
{
  auto const r(exec<I>(::std::forward<S>(stmt), ::std::forward<A>(args)...));
  assert(SQLITE_ROW == r);

  return get<T>(stmt, i);
}

template <typename T, int I = 1, typename S, typename ...A>
inline auto rexecget(S&& stmt, int const i = 0, A&& ...args) noexcept(
  noexcept(rexec<I>(::std::forward<S>(stmt), ::std::forward<A>(args)...),
    get<T>(stmt, i)
  )
)
{
  auto const r(rexec<I>(::std::forward<S>(stmt), ::std::forward<A>(args)...));
  assert(SQLITE_ROW == r);

  return get<T>(stmt, i);
}

template <typename T, int I = 1, typename D, typename A, typename ...B,
  typename = typename ::std::enable_if<
    ::std::is_same<remove_cvr_t<D>, sqlite3*>{} ||
    ::std::is_same<remove_cvr_t<D>, shared_db_t>{} ||
    ::std::is_same<remove_cvr_t<D>, unique_db_t>{}
  >::type
>
inline auto execget(D&& db, A&& a, int const i = 0, B&& ...args) noexcept(
  noexcept(
    execget<T, I>(
      make_unique(::std::forward<D>(db), ::std::forward<A>(a)),
      i,
      ::std::forward<B>(args)...
    )
  )
)
{
  return execget<T, I>(
    make_unique(::std::forward<D>(db), ::std::forward<A>(a)),
    i,
    ::std::forward<B>(args)...
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

  operator ::std::string() const &&
  {
    return {
      reinterpret_cast<char const*>(sqlite3_column_text(stmt_, i_)),
      ::std::string::size_type(sqlite3_column_bytes(stmt_, i_))
    };
  }

  operator charpair_t() const && noexcept
  {
    return {
      reinterpret_cast<char const*>(sqlite3_column_text(stmt_, i_)),
      sqlite3_column_bytes(stmt_, i_)
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
};

template <typename S,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline decltype(auto) operator|(S const& stmt, col&& c) noexcept
{
  assert(stmt);

  ::std::move(c).set_stmt(stmt.get());

  return c;
}

//changes/////////////////////////////////////////////////////////////////////
inline auto changes(sqlite3* const db) noexcept
{
  return sqlite3_changes(db);
}

//clear_bindings//////////////////////////////////////////////////////////////
inline auto clear_bindings(sqlite3_stmt* const stmt) noexcept
{
  return sqlite3_clear_bindings(stmt);
}

template <typename S,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline auto clear_bindings(S const& stmt) noexcept(
  noexcept(clear_bindings(stmt.get()))
)
{
  return clear_bindings(stmt.get());
}

//column_count////////////////////////////////////////////////////////////////
inline auto column_count(sqlite3_stmt* const stmt) noexcept
{
  return sqlite3_column_count(stmt);
}

template <typename S,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline auto column_count(S const& stmt) noexcept(
  noexcept(column_count(stmt.get()))
)
{
  return column_count(stmt.get());
}

//column_name/////////////////////////////////////////////////////////////////
inline auto column_name(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return sqlite3_column_name(stmt, i);
}

template <typename S,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline auto column_name(S const& stmt, int const i = 0) noexcept(
  noexcept(column_name(stmt.get(), i))
)
{
  return column_name(stmt.get(), i);
}

//column_name16///////////////////////////////////////////////////////////////
inline auto column_name16(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return static_cast<char16_t const*>(sqlite3_column_name16(stmt, i));
}

template <typename S,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline auto column_name16(S const& stmt, int const i = 0) noexcept(
  noexcept(column_name16(stmt.get(), i))
)
{
  return column_name16(stmt.get(), i);
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
inline auto open_shared(::std::string const& filename, A&& ...args) noexcept(
  noexcept(open_shared(filename.c_str(), ::std::forward<A>(args)...))
)
{
  return open_shared(filename.c_str(), ::std::forward<A>(args)...);
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
inline auto open_unique(::std::string const& filename, A&& ...args) noexcept(
  noexcept(open_unique(filename.c_str(), ::std::forward<A>(args)...))
)
{
  return open_unique(filename.c_str(), ::std::forward<A>(args)...);
}

//reset///////////////////////////////////////////////////////////////////////
inline auto reset(sqlite3_stmt* const stmt) noexcept
{
  return sqlite3_reset(stmt);
}

template <typename S,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline auto reset(S const& stmt) noexcept(
  noexcept(reset(stmt.get()))
)
{
  return reset(stmt.get());
}

//size////////////////////////////////////////////////////////////////////////
inline auto size(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return sqlite3_column_bytes(stmt, i);
}

template <typename S,
  typename = typename ::std::enable_if<is_stmt_t<S>{}>::type
>
inline auto size(S const& stmt, int const i = 0) noexcept(
  noexcept(size(stmt.get(), i))
)
{
  return size(stmt.get(), i);
}

template <typename ...A>
inline auto bytes(A&& ...args) noexcept(
  noexcept(size(::std::forward<A>(args)...))
)
{
  return size(::std::forward<A>(args)...);
}

//for_each////////////////////////////////////////////////////////////////////
namespace
{

template <typename ...A, typename F, typename S, ::std::size_t ...Is>
inline auto foreach_row_apply(S const& stmt, F const f, int const i,
  ::std::index_sequence<Is...> const) noexcept(
    noexcept(f(::std::declval<A>()...))
  )
{
  decltype(exec(stmt)) r;

  for (;;)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        if (f(
            get<::std::remove_const_t<::std::remove_reference_t<A> > >(
              stmt,
              i + count_types_n<Is, 0, A...>{}
            )...
          )
        )
        {
          continue;
        }
        else
        {
          break;
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

template <typename F, typename S, typename R, typename ...A>
inline auto foreach_row_fwd(S const& stmt, F&& f, int const i,
  R (* const)(A...)) noexcept(
    noexcept(
      foreach_row_apply<A...>(stmt,
        ::std::forward<F>(f),
        i,
        ::std::make_index_sequence<sizeof...(A)>()
      )
    )
  )
{
  return foreach_row_apply<A...>(stmt,
    ::std::forward<F>(f),
    i,
    ::std::make_index_sequence<sizeof...(A)>()
  );
}

template <typename F, typename S, typename R, typename ...A>
inline auto foreach_row_fwd(S const& stmt, F&& f, int const i,
  R (F::*)(A...)) noexcept(
    noexcept(
      foreach_row_apply<A...>(stmt,
        ::std::forward<F>(f),
        i,
        ::std::make_index_sequence<sizeof...(A)>()
      )
    )
  )
{
  return foreach_row_apply<A...>(stmt,
    ::std::forward<F>(f),
    i,
    ::std::make_index_sequence<sizeof...(A)>()
  );
}

template <typename F, typename S, typename R, typename ...A>
inline auto foreach_row_fwd(S const& stmt, F&& f, int const i,
  R (F::*)(A...) const) noexcept(
    noexcept(
      foreach_row_apply<A...>(stmt,
        ::std::forward<F>(f),
        i,
        ::std::make_index_sequence<sizeof...(A)>()
      )
    )
  )
{
  return foreach_row_apply<A...>(stmt,
    ::std::forward<F>(f),
    i,
    ::std::make_index_sequence<sizeof...(A)>()
  );
}

}

template <typename ...A, typename F, typename S,
  typename = typename ::std::enable_if<bool(sizeof...(A))>::type
>
inline auto foreach_row(S const& stmt, F&& f, int const i = 0) noexcept(
    noexcept(
      foreach_row_apply<A...>(stmt,
        ::std::forward<F>(f),
        i,
        ::std::make_index_sequence<sizeof...(A)>()
      )
    )
  )
{
  return foreach_row_apply<A...>(stmt,
    f,
    i,
    ::std::make_index_sequence<sizeof...(A)>()
  );
}

template <typename F, typename S>
inline typename ::std::enable_if<
  !::std::is_class<F>{},
  decltype(
    foreach_row_fwd(::std::declval<S>(), ::std::declval<F>(), 0)
  )
>::type
foreach_row(S const& stmt, F&& f, int const i = 0) noexcept(
    noexcept(foreach_row_fwd(stmt, ::std::forward<F>(f), i))
  )
{
  return foreach_row_fwd(stmt, ::std::forward<F>(f), i);
}

template <typename F, typename S>
inline typename ::std::enable_if<
  ::std::is_class<F>{},
  decltype(
    foreach_row_fwd(::std::declval<S>(), ::std::declval<F>(), 0,
      &F::operator()
    )
  )
>::type
foreach_row(S const& stmt, F&& f, int const i = 0) noexcept(
    noexcept(foreach_row_fwd(stmt, ::std::forward<F>(f), i, &F::operator()))
  )
{
  return foreach_row_fwd(stmt, ::std::forward<F>(f), i, &F::operator());
}

template <typename F, typename S>
inline auto foreach_stmt(S const& stmt, F const f) noexcept(noexcept(f()))
{
  decltype(exec(stmt)) r;

  for (;;)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        if (f())
        {
          continue;
        }
        else
        {
          break;
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

//container_push//////////////////////////////////////////////////////////////
template <typename FP, FP fp, typename C, typename S>
inline auto container_push(S&& s, C& c, int const i)
{
  decltype(exec(::std::forward<S>(s))) r;

  for (;;)
  {
    switch (r = exec(::std::forward<S>(s)))
    {
      case SQLITE_ROW:
        (c.*fp)(get<typename C::value_type>(::std::forward<S>(s), i));

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
  decltype(exec(::std::forward<S>(s))) r(SQLITE_DONE);

  for (T j{}; j != n; ++j)
  {
    switch (r = exec(::std::forward<S>(s)))
    {
      case SQLITE_ROW:
        (c.*fp)(get<typename C::value_type>(::std::forward<S>(s), i));

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

//emplace/////////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto emplace(S&& s, C& c, int const i = 0)
{
  return container_push<
    decltype(&C::template emplace<typename C::value_type>),
    &C::template emplace<typename C::value_type>
  >(::std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto emplace_n(S const& s, C& c, T const n, int const i = 0)
{
  return container_push<
    decltype(&C::template emplace<typename C::value_type>),
    &C::template emplace<typename C::value_type>
  >(::std::forward<S>(s), c, n, i);
}

//emplace_back////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto emplace_back(S&& s, C& c, int const i = 0)
{
  return container_push<
    decltype(&C::template emplace_back<typename C::value_type>),
    &C::template emplace_back<typename C::value_type>
  >(::std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto emplace_back_n(S&& s, C& c, T const n, int const i = 0)
{
  return container_push<
    decltype(&C::template emplace_back<typename C::value_type>),
    &C::template emplace_back<typename C::value_type>
  >(::std::forward<S>(s), c, n, i);
}

//insert//////////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto insert(S&& s, C& c, int const i = 0)
{
  return container_push<
    decltype(&C::template insert<typename C::value_type>),
    &C::template insert<typename C::value_type>
  >(::std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto insert_n(S const& s, C& c, T const n, int const i = 0)
{
  return container_push<
    decltype(&C::template insert<typename C::value_type>),
    &C::template insert<typename C::value_type>
  >(::std::forward<S>(s), c, n, i);
}

//push_back///////////////////////////////////////////////////////////////////
template <typename C, typename S>
inline auto push_back(S&& s, C& c, int const i = 0)
{
  return container_push<
    decltype(&C::template push_back<typename C::value_type>),
    &C::template push_back<typename C::value_type>
  >(::std::forward<S>(s), c, i);
}

template <typename C, typename S, typename T>
inline auto push_back_n(S&& s, C& c, T const n, int const i = 0)
{
  return container_push<
    decltype(&C::template push_back<typename C::value_type>),
    &C::template push_back<typename C::value_type>
  >(::std::forward<S>(s), c, n, i);
}

}

#endif // SQLITEUTILS_HPP

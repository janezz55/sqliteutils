#ifndef SQLITEUTILS_HPP
# define SQLITEUTILS_HPP
# pragma once

#include "sqlite3/sqlite3.h"

#include <cassert>

#include <memory>

#include <string>

#include <type_traits>

#include <utility>

namespace sqlite
{

using blobpair_t = ::std::pair<void const* const, sqlite3_uint64 const>;

using charpair_t = ::std::pair<char const* const, sqlite3_uint64 const>;

using char16pair_t = ::std::pair<char16_t const* const, int const>;

using nullpair_t = ::std::pair<::std::nullptr_t const, sqlite3_uint64 const>;

namespace detail
{

struct swallow
{
  template <typename ...T>
  explicit swallow(T&& ...) noexcept
  {
  }
};

struct deleter
{
  void operator()(sqlite3_stmt* const p) const noexcept
  {
    sqlite3_finalize(p);
  }
};

template <int I>
inline auto set(sqlite3_stmt* const stmt, blobpair_t const& v) noexcept
{
  return sqlite3_bind_blob64(stmt, I, v.first, v.second, SQLITE_TRANSIENT);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, double const v) noexcept
{
  return sqlite3_bind_double(stmt, I, v);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, unsigned const v) noexcept
{
  return sqlite3_bind_int(stmt, I, v);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, unsigned long const v) noexcept
{
  return sqlite3_bind_int64(stmt, I, v);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, unsigned long long const v) noexcept
{
  return sqlite3_bind_int64(stmt, I, v);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, int const v) noexcept
{
  return sqlite3_bind_int(stmt, I, v);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, long int const v) noexcept
{
  return sqlite3_bind_int64(stmt, I, v);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, long long int const v) noexcept
{
  return sqlite3_bind_int64(stmt, I, v);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, ::std::nullptr_t const) noexcept
{
  return sqlite3_bind_null(stmt, I);
}

template <int I, ::std::size_t N>
inline auto set(sqlite3_stmt* const stmt, char const (&v)[N]) noexcept
{
  return sqlite3_bind_text64(stmt, I, v, N, SQLITE_STATIC, SQLITE_UTF8);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, charpair_t const& v) noexcept
{
  return sqlite3_bind_text64(stmt, I, v.first, v.second, SQLITE_STATIC,
    SQLITE_UTF8);
}

template <int I, typename T,
  typename = typename std::enable_if<
    std::is_same<T, char>{}
  >::type
>
inline auto set(sqlite3_stmt* const stmt, T const* const& v) noexcept
{
  return sqlite3_bind_text64(stmt, I, v, -1, SQLITE_STATIC,
    SQLITE_UTF8);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, ::std::string const& v) noexcept
{
  return sqlite3_bind_text64(stmt, I, v.c_str(), v.size(), SQLITE_TRANSIENT,
    SQLITE_UTF8);
}

template <int I, ::std::size_t N>
inline auto set(sqlite3_stmt* const stmt, char16_t const (&v)[N]) noexcept
{
  return sqlite3_bind_text(stmt, I, v, N, SQLITE_STATIC);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, char16_t const* v) noexcept
{
  return sqlite3_bind_text16(stmt, I, v, -1, SQLITE_TRANSIENT);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, char16pair_t const& v) noexcept
{
  return sqlite3_bind_text16(stmt, I, v.first, v.second, SQLITE_TRANSIENT);
}

template <int I>
inline auto set(sqlite3_stmt* const stmt, nullpair_t const& v) noexcept
{
  return sqlite3_bind_zeroblob64(stmt, I, v.second);
}

template <int I, ::std::size_t ...Is, typename ...A>
void set(sqlite3_stmt* const stmt, ::std::index_sequence<Is...> const,
  A&& ...args)
{
  swallow{set<I + Is>(stmt, args)...};
}

}

using stmt_t = ::std::unique_ptr<sqlite3_stmt, detail::deleter>;

//set/////////////////////////////////////////////////////////////////////////
template <int I = 1, typename ...A>
inline auto set(sqlite3_stmt* const stmt, A&& ...args) noexcept
{
  detail::set<I>(stmt,
    ::std::make_index_sequence<sizeof...(A)>(),
    ::std::forward<A>(args)...
  );

  return stmt;
}

template <int I = 1, typename ...A>
inline auto rset(sqlite3_stmt* const stmt, A&& ...args) noexcept
{
  sqlite3_reset(stmt);

  return set<I>(stmt, ::std::forward<A>(args)...);
}

//make_stmt///////////////////////////////////////////////////////////////////
template <typename T,
  typename = typename std::enable_if<
    std::is_same<T, char>{}
  >::type
>
inline stmt_t make_stmt(sqlite3* const db, T const* const& a,
  int const size = -1) noexcept
{
  sqlite3_stmt* stmt;

#ifndef NDEBUG
  auto const result(sqlite3_prepare_v2(db, a, size, &stmt, nullptr));
  assert(SQLITE_OK == result);

  return stmt_t(stmt);
#else
  return stmt_t(sqlite3_prepare_v2(db, a, size, &stmt, nullptr));
#endif // NDEBUG
}

template <::std::size_t N>
inline stmt_t make_stmt(sqlite3* const db, char const (&a)[N]) noexcept
{
  sqlite3_stmt* stmt;

#ifndef NDEBUG
  auto const result(sqlite3_prepare_v2(db, a, N, &stmt, nullptr));
  assert(SQLITE_OK == result);

  return stmt_t(stmt);
#else
  return stmt_t(sqlite3_prepare_v2(db, a, N, &stmt, nullptr));
#endif // NDEBUG
}

inline stmt_t make_stmt(sqlite3* const db, ::std::string const& a) noexcept
{
  return make_stmt(db, a.c_str(), a.size());
}

//exec ///////////////////////////////////////////////////////////////////////
template <int I = 1>
inline auto exec(sqlite3_stmt* const stmt) noexcept
{
  return sqlite3_step(stmt);
}

template <int I = 1, typename ...A>
inline auto exec(sqlite3_stmt* const stmt, A&& ...args) noexcept
{
  return exec(set<I>(stmt, ::std::forward<A>(args)...));
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
  return exec(rset<I>(stmt, ::std::forward<A>(args)...));
}

template <int I = 1, typename ...A>
inline auto exec(stmt_t const& stmt, A&& ...args) noexcept(
  noexcept(exec(stmt.get(), ::std::forward<A>(args)...))
)
{
  return exec<I>(
    stmt.get(),
    ::std::forward<A>(args)...
  );
}

template <int I = 1, typename ...A>
inline auto rexec(stmt_t const& stmt, A&& ...args) noexcept(
  noexcept(rexec(stmt.get(), ::std::forward<A>(args)...))
)
{
  return rexec<I>(
    stmt.get(),
    ::std::forward<A>(args)...
  );
}

template <int I = 1, typename T, typename ...A,
  typename = typename std::enable_if<
    std::is_same<T, char>{}
  >::type
>
inline auto exec(sqlite3* const db, T const* const& a, A&& ...args) noexcept
{
  return exec<I>(
    make_stmt(db, a),
    ::std::forward<A>(args)...
  );
}

template <int I = 1, ::std::size_t N, typename ...A>
inline auto exec(sqlite3* const db, char const (&a)[N], A&& ...args) noexcept
{
  return exec<I>(
    make_stmt<N>(db, a),
    ::std::forward<A>(args)...
  );
}

template <int I = 1, ::std::size_t N, typename ...A>
inline auto exec(sqlite3* const db, ::std::string const& a,
  A&& ...args) noexcept
{
  return exec<I>(
    make_stmt(db, a.c_str(), a.size()),
    ::std::forward<A>(args)...
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
  ::std::is_same<T, void const*>{},
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return sqlite3_column_blob(stmt, i);
}

template <typename>
struct is_std_pair : ::std::false_type { };

template <class T1, class T2>
struct is_std_pair<::std::pair<T1, T2> > : ::std::true_type { };

template <typename T>
inline typename ::std::enable_if<
  is_std_pair<T>{},
  T
>::type
get(sqlite3_stmt* const stmt, int const i = 0) noexcept(
  noexcept(
    T(
      get<typename T::first_type>(stmt, i),
      get<typename T::second_type>(stmt, i + 1)
    )
  )
)
{
  return {
    get<typename T::first_type>(stmt, i),
    get<typename T::second_type>(stmt, i + 1)
  };
}

template <typename T>
inline auto get(stmt_t const& stmt, int const i = 0) noexcept(
  noexcept(get<T>(stmt.get(), i))
)
{
  return get<T>(stmt.get(), i);
}

//column_count////////////////////////////////////////////////////////////////
inline auto column_count(sqlite3_stmt* const stmt) noexcept
{
  return sqlite3_column_count(stmt);
}

inline auto column_count(stmt_t const& stmt) noexcept(
  noexcept(column_count(stmt.get()))
)
{
  return column_count(stmt.get());
}

//size////////////////////////////////////////////////////////////////////////
inline auto size(sqlite3_stmt* const stmt, int const i = 0) noexcept
{
  return sqlite3_column_bytes(stmt, i);
}

inline auto size(stmt_t const& stmt, int const i = 0) noexcept(
  noexcept(size(stmt.get(), i))
)
{
  return size(stmt.get(), i);
}

}

#endif // SQLITEUTILS_HPP

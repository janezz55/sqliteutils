#ifndef SQLITEUTILS_HPP
# define SQLITEUTILS_HPP
# pragma once

#include <cassert>

#include <memory>

#include <string>

#include <type_traits>

#include <utility>

#include "sqlite3/sqlite3.h"

namespace sqlite
{

using blobpair_t = ::std::pair<void const* const, int const>;

using charpair_t = ::std::pair<char const* const, int const>;

using char16pair_t = ::std::pair<char16_t const* const, int const>;

using nullpair_t = ::std::pair<::std::nullptr_t const, int const>;

namespace detail
{

inline auto bind(sqlite3_stmt* const stmt, int const i,
  blobpair_t const& v) noexcept
{
  return sqlite3_bind_blob(stmt, i, v.first, v.second, SQLITE_TRANSIENT);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  double const v) noexcept
{
  return sqlite3_bind_double(stmt, i, v);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  unsigned const v) noexcept
{
  return sqlite3_bind_int(stmt, i, v);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  unsigned long const v) noexcept
{
  return sqlite3_bind_int64(stmt, i, v);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  unsigned long long const v) noexcept
{
  return sqlite3_bind_int64(stmt, i, v);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  int const v) noexcept
{
  return sqlite3_bind_int(stmt, i, v);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  long int const v) noexcept
{
  return sqlite3_bind_int64(stmt, i, v);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  long long int const v) noexcept
{
  return sqlite3_bind_int64(stmt, i, v);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  ::std::nullptr_t const) noexcept
{
  return sqlite3_bind_null(stmt, i);
}

template <::std::size_t N>
inline auto bind(sqlite3_stmt* const stmt, int const i,
  char const (&v)[N]) noexcept
{
  return sqlite3_bind_text(stmt, i, v, N, SQLITE_STATIC);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  charpair_t const& v) noexcept
{
  return sqlite3_bind_text(stmt, i, v.first, v.second, SQLITE_TRANSIENT);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  char const* const v) noexcept
{
  return sqlite3_bind_text(stmt, i, v, -1, SQLITE_TRANSIENT);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  ::std::string const& v) noexcept
{
  return sqlite3_bind_text(stmt, i, v.c_str(), v.size(), SQLITE_TRANSIENT);
}

template <::std::size_t N>
inline auto bind(sqlite3_stmt* const stmt, int const i,
  char16_t const (&v)[N]) noexcept
{
  return sqlite3_bind_text(stmt, i, v, N, SQLITE_STATIC);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  char16_t const* v) noexcept
{
  return sqlite3_bind_text16(stmt, i, v, -1, SQLITE_TRANSIENT);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  char16pair_t const& v) noexcept
{
  return sqlite3_bind_text16(stmt, i, v.first, v.second, SQLITE_TRANSIENT);
}

inline auto bind(sqlite3_stmt* const stmt, int const i,
  nullpair_t const& v) noexcept
{
  return sqlite3_bind_zeroblob(stmt, i, v.second);
}

struct deleter
{
  void operator()(sqlite3_stmt* const p) const noexcept
  {
    sqlite3_finalize(p);
  }
};

}

//////////////////////////////////////////////////////////////////////////////
template <typename T>
inline auto bind(sqlite3_stmt* const stmt, int const i, T&& a) noexcept
{
  return detail::bind(stmt, i, ::std::forward<T>(a));
}

//////////////////////////////////////////////////////////////////////////////
template <typename T, typename ...A>
inline int bind(sqlite3_stmt* const stmt, int const i, T&& a, A&& ...args)
  noexcept
{
  auto const r(detail::bind(stmt, i, ::std::forward<T>(a)));

  return SQLITE_OK == r ?
    bind(stmt, i + 1, ::std::forward<A>(args)...) :
    r;
}

//////////////////////////////////////////////////////////////////////////////
template <typename ...A>
inline auto bind_front(sqlite3_stmt* const stmt, A&& ...args) noexcept
{
  return bind(stmt, 1, ::std::forward<A>(args)...);
}

using stmt_t = ::std::unique_ptr<sqlite3_stmt, detail::deleter>;

//////////////////////////////////////////////////////////////////////////////
inline auto exec(sqlite3* const db, char const* const a) noexcept
{
  return sqlite3_exec(db, a, nullptr, nullptr, nullptr);
}

//////////////////////////////////////////////////////////////////////////////
template <::std::size_t N>
inline auto exec(sqlite3* const db, char const (&a)[N]) noexcept
{
  return exec(db, &*a);
}

//////////////////////////////////////////////////////////////////////////////
inline auto exec(sqlite3* const db, ::std::string const& a) noexcept
{
  return exec(db, a.c_str());
}

//////////////////////////////////////////////////////////////////////////////
inline auto exec(sqlite3_stmt* const stmt) noexcept
{
  sqlite3_reset(stmt);

  return sqlite3_step(stmt);
}

//////////////////////////////////////////////////////////////////////////////
template <typename ...A>
inline auto exec(sqlite3_stmt* const stmt, int const i, A&& ...args) noexcept
{
  sqlite3_reset(stmt);

  auto result(bind(stmt, i, ::std::forward<A>(args)...));
  assert(SQLITE_OK == result);

  return sqlite3_step(stmt);
}

//////////////////////////////////////////////////////////////////////////////
template <typename ...A>
inline auto exec(stmt_t const& stmt, A&& ...args) noexcept
{
  return exec(stmt.get(), ::std::forward<A>(args)...);
}

//////////////////////////////////////////////////////////////////////////////
template <typename ...A>
inline auto exec_front(sqlite3_stmt* const stmt, A&& ...args) noexcept
{
  sqlite3_reset(stmt);

  auto result(bind_front(stmt, ::std::forward<A>(args)...));
  assert(SQLITE_OK == result);

  return sqlite3_step(stmt);
}

//////////////////////////////////////////////////////////////////////////////
template <typename ...A>
inline auto exec_front(stmt_t const& stmt, A&& ...args) noexcept
{
  return exec_front(stmt.get(), ::std::forward<A>(args)...);
}

//////////////////////////////////////////////////////////////////////////////
inline stmt_t make_stmt(sqlite3* const db, char const* const a) noexcept
{
  sqlite3_stmt* stmt{};

  auto const result(sqlite3_prepare_v2(db, a, -1, &stmt, nullptr));
  assert(SQLITE_OK == result);

  return stmt_t(stmt);
}

//////////////////////////////////////////////////////////////////////////////
template <::std::size_t N>
inline stmt_t make_stmt(sqlite3* const db, char const (&a)[N]) noexcept
{
  return make_stmt(db, &*a);
}

//////////////////////////////////////////////////////////////////////////////
inline stmt_t make_stmt(sqlite3* const db, ::std::string const& a) noexcept
{
  return make_stmt(db, a.c_str());
}

//////////////////////////////////////////////////////////////////////////////
template <typename ...A>
inline auto exec(sqlite3* const db, char const* const a, int const i,
  A&& ...args) noexcept
{
  return exec(make_stmt(db, a), i, ::std::forward<A>(args)...);
}

//////////////////////////////////////////////////////////////////////////////
template <::std::size_t N, typename ...A>
inline auto exec(sqlite3* const db, char const (&a)[N], int const i,
  A&& ...args) noexcept
{
  return exec(db, &*a, i, ::std::forward<A>(args)...);
}

//////////////////////////////////////////////////////////////////////////////
template <::std::size_t N, typename ...A>
inline auto exec(sqlite3* const db, ::std::string const& a, int const i,
  A&& ...args) noexcept
{
  return exec(db, a.c_str(), i, ::std::forward<A>(args)...);
}

//////////////////////////////////////////////////////////////////////////////
template <::std::size_t N, typename ...A>
inline auto exec_front(sqlite3* const db, char const (&a)[N],
  A&& ...args) noexcept
{
  return exec(db, &*a, 1, ::std::forward<A>(args)...);
}

//////////////////////////////////////////////////////////////////////////////
template <::std::size_t N, typename ...A>
inline auto exec_front(sqlite3* const db, ::std::string const& a,
  A&& ...args) noexcept
{
  return exec(db, a.c_str(), 1, ::std::forward<A>(args)...);
}

//////////////////////////////////////////////////////////////////////////////
class out
{
  int i_;
  sqlite3_stmt* const stmt_;

public:
  explicit out(stmt_t const& stmt, int const i = 0) noexcept :
    i_(i),
    stmt_(stmt.get())
  {
  }

  auto index() noexcept -> decltype((i_)) { return i_; }

  auto stmt() const noexcept -> decltype(stmt_) { return stmt_; }
};

template <typename T>
inline typename ::std::enable_if<
  ::std::is_integral<T>{} && (sizeof(T) <= sizeof(int)),
  out&
>::type
operator>>(out& o, T& v) noexcept
{
  v = sqlite3_column_int(o.stmt(), o.index()++);

  return o;
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_integral<T>{} && (sizeof(T) > sizeof(int)),
  out&
>::type
operator>>(out& o, T& v) noexcept
{
  v = sqlite3_column_int64(o.stmt(), o.index()++);

  return o;
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_floating_point<T>{},
  out&
>::type
operator>>(out& o, T& v) noexcept
{
  v = sqlite3_column_double(o.stmt(), o.index()++);

  return o;
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_same<typename ::std::decay<T>::type, char const*>{},
  out&
>::type
operator>>(out& o, T& v) noexcept
{
  v = reinterpret_cast<char const*>(
    sqlite3_column_text(o.stmt(), o.index()++)
  );

  return o;
}

template <typename T>
inline typename ::std::enable_if<
  ::std::is_same<typename ::std::decay<T>::type, void const*>{},
  out&
>::type
operator>>(out& o, T& v) noexcept
{
  v = sqlite3_column_blob(o.stmt(), o.index()++);

  return o;
}

}

#endif // SQLITEUTILS_HPP

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

using char16pair_t = ::std::pair<char16_t const* const, sqlite3_uint64 const>;

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
inline void set(sqlite3_stmt* const stmt, blobpair_t const& v) noexcept
{
  sqlite3_bind_blob64(stmt, I, v.first, v.second, SQLITE_STATIC);
}

template <int I>
inline void set(sqlite3_stmt* const stmt, double const v) noexcept
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

using stmt_t = ::std::unique_ptr<sqlite3_stmt, detail::deleter>;

//set/////////////////////////////////////////////////////////////////////////
template <int I = 1, typename ...A>
inline void set(sqlite3_stmt* const stmt, A&& ...args) noexcept
{
  detail::set<I>(stmt,
    ::std::make_index_sequence<sizeof...(A)>(),
    ::std::forward<A>(args)...
  );
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
#else
  sqlite3_prepare_v2(db, a, size, &stmt, nullptr);
#endif // NDEBUG
  return stmt_t(stmt);
}

template <::std::size_t N>
inline stmt_t make_stmt(sqlite3* const db, char const (&a)[N]) noexcept
{
  sqlite3_stmt* stmt;

#ifndef NDEBUG
  auto const result(sqlite3_prepare_v2(db, a, N, &stmt, nullptr));
  assert(SQLITE_OK == result);
#else
  sqlite3_prepare_v2(db, a, N, &stmt, nullptr);
#endif // NDEBUG
  return stmt_t(stmt);
}

inline stmt_t make_stmt(sqlite3* const db, ::std::string const& a) noexcept
{
  return make_stmt(db, a.c_str(), a.size());
}

// forwarders
template <typename T, typename ...A>
inline auto make_stmt(::std::shared_ptr<T> const& db, A&& ...args) noexcept(
  noexcept(make_stmt(db.get(), ::std::forward<A>(args)...))
)
{
  return make_stmt(db.get(), ::std::forward<A>(args)...);
}

template <typename T, typename D, typename ...A>
inline auto make_stmt(::std::unique_ptr<T, D> const& db,
  A&& ...args) noexcept(
    noexcept(make_stmt(db.get(), ::std::forward<A>(args)...))
  )
{
  return make_stmt(db.get(), ::std::forward<A>(args)...);
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

  set<I>(stmt, ::std::forward<A>(args)...);

  return exec(stmt);
}

// forwarders
template <int I = 1, typename ...A>
inline auto exec(stmt_t const& stmt, A&& ...args) noexcept(
  noexcept(exec(stmt.get(), ::std::forward<A>(args)...))
)
{
  return exec<I>(stmt.get(), ::std::forward<A>(args)...);
}

template <int I = 1, typename ...A>
inline auto rexec(stmt_t const& stmt, A&& ...args) noexcept(
  noexcept(rexec(stmt.get(), ::std::forward<A>(args)...))
)
{
  return rexec<I>(stmt.get(), ::std::forward<A>(args)...);
}

template <int I = 1, typename A, typename ...B>
inline auto exec(sqlite3* const db, A&& a, B&& ...args) noexcept(
  noexcept(
    exec<I>(
      make_stmt(db, ::std::forward<A>(a)),
      ::std::forward<B>(args)...
    )
  )
)
{
  return exec<I>(
    make_stmt(db, ::std::forward<A>(a)),
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
template <typename T, typename ...A>
inline auto exec(::std::shared_ptr<T> const& db, A&& ...args) noexcept(
  noexcept(exec(db.get(), ::std::forward<A>(args)...))
)
{
  return exec(db.get(), ::std::forward<A>(args)...);
}

template <typename T, typename D, typename ...A>
inline auto exec(::std::unique_ptr<T, D> const& db, A&& ...args) noexcept(
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
get(sqlite3_stmt* const stmt, int const i = 0) noexcept
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

//execget/////////////////////////////////////////////////////////////////////
template <typename T, typename S, typename ...A>
inline auto execget(S&& stmt, int const i = 0, A&& ...args)
{
  auto const r(exec(::std::forward<S>(stmt), ::std::forward<A>(args)...));
  assert(SQLITE_ROW == r);

  return get<T>(stmt, i);
}

template <typename T, typename S, typename ...A>
inline auto rexecget(S&& stmt, int const i = 0, A&& ...args)
{
  auto const r(rexec(::std::forward<S>(stmt), ::std::forward<A>(args)...));
  assert(SQLITE_ROW == r);

  return get<T>(stmt, i);
}

template <typename T, typename D, typename A, typename ...B>
inline auto execget(D&& db, A&& a, int const i = 0, B&& ...args)
{
  return execget<T>(make_stmt(::std::forward<D>(db), ::std::forward<A>(a)),
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

  operator ::std::string() const && noexcept
  {
    return {
      reinterpret_cast<char const*>(sqlite3_column_text(stmt_, i_)),
      ::std::string::size_type(sqlite3_column_bytes(stmt_, i_))
    };
  }

  operator void const*() const && noexcept
  {
    return sqlite3_column_blob(stmt_, i_);
  }
};

inline col const& operator|(stmt_t const& stmt, col&& c) noexcept
{
  assert(stmt);

#ifndef NDEBUG
  auto const r(exec(stmt));
  assert(SQLITE_ROW == r);
#else
  exec(stmt);
#endif // NDEBUG

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

inline auto clear_bindings(stmt_t const& stmt) noexcept(
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

inline auto column_count(stmt_t const& stmt) noexcept(
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

inline auto column_name(stmt_t const& stmt, int const i = 0) noexcept(
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

inline auto column_name16(stmt_t const& stmt, int const i = 0) noexcept(
  noexcept(column_name16(stmt.get(), i))
)
{
  return column_name16(stmt.get(), i);
}

//reset///////////////////////////////////////////////////////////////////////
inline auto reset(sqlite3_stmt* const stmt) noexcept
{
  return sqlite3_reset(stmt);
}

inline auto reset(stmt_t const& stmt) noexcept(
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

inline auto size(stmt_t const& stmt, int const i = 0) noexcept(
  noexcept(size(stmt.get(), i))
)
{
  return size(stmt.get(), i);
}

//for_each////////////////////////////////////////////////////////////////////
namespace
{

template <typename ...A, typename F, ::std::size_t ...Is>
inline void foreach_row_apply(stmt_t const& stmt, F&& f, int const i,
  ::std::index_sequence<Is...> const) noexcept(
    noexcept(f(::std::declval<A>()...))
  )
{
  for (;;)
  {
    switch (exec(stmt))
    {
      case SQLITE_ROW:
        if (f(
            get<::std::remove_const_t<::std::remove_reference_t<A> > >(
              stmt, i + Is
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
}

template <typename F, typename R, typename ...A>
inline void foreach_row_fwd(stmt_t const& stmt, F&& f,
  int const i, R (F::*)(A...)) noexcept(
    noexcept(
      foreach_row_apply<A...>(stmt,
        ::std::forward<F>(f),
        i,
        ::std::make_index_sequence<sizeof...(A)>()
      )
    )
  )
{
  foreach_row_apply<A...>(stmt,
    ::std::forward<F>(f),
    i,
    ::std::make_index_sequence<sizeof...(A)>()
  );
}

template <typename F, typename R, typename ...A>
inline void foreach_row_fwd(stmt_t const& stmt, F&& f,
  int const i, R (F::*)(A...) const) noexcept(
    noexcept(
      foreach_row_apply<A...>(stmt,
        ::std::forward<F>(f),
        i,
        ::std::make_index_sequence<sizeof...(A)>()
      )
    )
  )
{
  foreach_row_apply<A...>(stmt,
    ::std::forward<F>(f),
    i,
    ::std::make_index_sequence<sizeof...(A)>()
  );
}

}

template <typename F>
inline void foreach_row(stmt_t const& stmt, F&& f, int const i = 0) noexcept(
  noexcept(foreach_row_fwd(stmt, ::std::forward<F>(f), i, &F::operator()))
)
{
  foreach_row_fwd(stmt, ::std::forward<F>(f), i, &F::operator());
}

template <typename ...A, typename F,
  typename = typename ::std::enable_if<bool(sizeof...(A))>::type
>
inline void foreach_row(stmt_t const& stmt, F&& f, int const i = 0) noexcept(
  noexcept(
    foreach_row_apply<A...>(stmt,
      ::std::forward<F>(f),
      i,
      ::std::make_index_sequence<sizeof...(A)>()
    )
  )
)
{
  foreach_row_apply<A...>(stmt,
    ::std::forward<F>(f),
    i,
    ::std::make_index_sequence<sizeof...(A)>()
  );
}

template <typename F>
void foreach_stmt(stmt_t const& stmt, F&& f) noexcept(noexcept(f()))
{
  for (;;)
  {
    switch (exec(stmt))
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
}

//emplace/////////////////////////////////////////////////////////////////////
template <typename C>
inline void emplace(stmt_t const& stmt, C& c, int const i = 0)
{
  decltype(exec(stmt)) r(SQLITE_DONE);

  for (;;)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        c.emplace(get<typename C::value_type>(stmt, i));

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

template <typename C, typename T>
inline auto emplace_n(stmt_t const& stmt, C& c, T const n, int const i = 0)
{
  decltype(exec(stmt)) r(SQLITE_DONE);

  for (T j{}; j != n; ++j)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        c.emplace(get<typename C::value_type>(stmt, i));

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

//emplace_back////////////////////////////////////////////////////////////////
template <typename C>
inline auto emplace_back(stmt_t const& stmt, C& c, int const i = 0)
{
  decltype(exec(stmt)) r(SQLITE_DONE);

  for (;;)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        c.emplace_back(get<typename C::value_type>(stmt, i));

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

template <typename C, typename T>
inline bool emplace_back_n(stmt_t const& stmt, C& c, T const n,
  int const i = 0)
{
  decltype(exec(stmt)) r(SQLITE_DONE);

  for (T j{}; j != n; ++j)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        c.emplace_back(get<typename C::value_type>(stmt, i));

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

//insert//////////////////////////////////////////////////////////////////////
template <typename C>
inline void insert(stmt_t const& stmt, C& c, int const i = 0)
{
  decltype(exec(stmt)) r(SQLITE_DONE);

  for (;;)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        c.insert(get<typename C::value_type>(stmt, i));

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

template <typename C, typename T>
inline auto insert_n(stmt_t const& stmt, C& c, T const n, int const i = 0)
{
  decltype(exec(stmt)) r(SQLITE_DONE);

  for (T j{}; j != n; ++j)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        c.insert(get<typename C::value_type>(stmt, i));

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

//push_back///////////////////////////////////////////////////////////////////
template <typename C>
inline void push_back(stmt_t const& stmt, C& c, int const i = 0)
{
  decltype(exec(stmt)) r(SQLITE_DONE);

  for (;;)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        c.push_back(get<typename C::value_type>(stmt, i));

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

template <typename C, typename T>
inline auto push_back_n(stmt_t const& stmt, C& c, T const n, int const i = 0)
{
  decltype(exec(stmt)) r(SQLITE_DONE);

  for (T j{}; j != n; ++j)
  {
    switch (r = exec(stmt))
    {
      case SQLITE_ROW:
        c.push_back(get<typename C::value_type>(stmt, i));

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

#endif // SQLITEUTILS_HPP

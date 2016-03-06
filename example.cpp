#include <iostream>

#include <vector>

#include "sqliteutils.hpp"

int main(int, char*[])
{
  auto const db(::sqlite::open("example.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));

  ::sqlite::exec(db,
    "CREATE TABLE COMPANY("
    "ID INT PRIMARY KEY     NOT NULL,"
    "NAME           TEXT    NOT NULL,"
    "AGE            INT     NOT NULL,"
    "ADDRESS        CHAR(50),"
    "SALARY         REAL);"
    "DELETE FROM COMPANY"
  );

  ::sqlite::exec(db,
    "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
    "VALUES(1, 'Paul', 32, 'California', 20000.00);"
    "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
    "VALUES(2, 'Allen', 25, 'Texas', 15000.00 );"
    "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
    "VALUES(3, 'Teddy', 23, 'Norway', 20000.00 );"
    "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
    "VALUES(4, 'Mark', 25, 'Rich-Mond ', 65000.00)"
  );

  auto const stmt(::sqlite::make_stmt(db, "SELECT NAME,AGE,ADDRESS,SALARY FROM COMPANY"));

  ::sqlite::foreach_row(stmt,
    [](::std::string const& name, unsigned const age,
      ::std::string const& address, double const salary) noexcept
    {
      ::std::cout << name << " " << age << " " << address << " " << salary << ::std::endl;

      return true;
    }
  );

  ::sqlite::reset(stmt);

  ::std::vector<::std::tuple<::std::string, unsigned, ::std::string, double> > v;

  ::sqlite::emplace_back(stmt, v);

  for (auto& t: v)
  {
    ::std::cout << ::std::get<0>(t) << " " << ::std::get<1>(t) << " " << ::std::get<2>(t) << " " << ::std::get<3>(t) << ::std::endl;
  }

  return 0;
}

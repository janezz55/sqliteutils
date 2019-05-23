## Description
A lightweight C++17 wrapper library for sqlite3. The goals of the library are:
- exception safety (no exceptions thrown),
- code verbosity,
- resource management,
- transparency,
- type safety,
- performance.

Please create issues to request new features.

## Example
```c++
#include <iostream>

#include <vector>

#include "sqliteutils.hpp"

using namespace squ::literals;

int main(int, char*[])
{
  auto const db(squ::open_unique("example.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));

  "DROP TABLE IF EXISTS COMPANY;"
  "CREATE TABLE COMPANY("
  "ID INT PRIMARY KEY     NOT NULL,"
  "NAME           TEXT    NOT NULL,"
  "AGE            INT     NOT NULL,"
  "ADDRESS        CHAR(50),"
  "SALARY         REAL);"
  "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
  "VALUES(1, 'Paul', 32, 'California', 20000.00);"
  "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
  "VALUES(2, 'Allen', 25, 'Texas', 15000.00 );"
  "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
  "VALUES(3, 'Teddy', 23, 'Norway', 20000.00 );"
  "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
  "VALUES(4, 'Mark', 25, 'Rich-Mond ', 65000.00)"_squ.execmulti(db);

  auto const stmt(
    "SELECT NAME,AGE,ADDRESS,SALARY FROM COMPANY"_squ.unique(db)
  );

  squ::foreach_row(stmt,
    [](std::string const& name, unsigned const age,
      std::string const& address, double const salary) noexcept
    {
      std::cout << name << " " << age << " " << address << " " << salary << std::endl;
    }
  );

  squ::reset(stmt);

  std::vector<std::tuple<std::string, unsigned, std::string, double> > v;

  squ::emplace_back(stmt, v);

  for (auto& t: v)
  {
    std::cout << std::get<0>(t) << " " << std::get<1>(t) << " " << std::get<2>(t) << " " << std::get<3>(t) << std::endl;
  }

  return 0;
}
```

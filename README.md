## Description
A lightweight C++17 wrapper library for sqlite3. The goals of the library are:
- exception safety (no exceptions thrown),
- code verbosity,
- resource management,
- transparency,
- type safety,
- performance.

Please create issues to request new features. Try it [online](http://htmlpreview.github.io/?https://github.com/user1095108/examples/blob/master/testsqu.html).
## Discussion
The library is meant to relieve the need for a plethora of bookkeeping boiler-plate code, that often appears in .c files using the sqlite library. The smart pointers should delete statements and database connections, that you don't need anymore. In addition, the binding code should now be simplified. You can access the wrapper through string literals, or through wrapped function calls as in:
```c++
"SELECT 'lol'"_squ.execget<std::string>(db).value();
```
or
```c++
squ::execget<std::string>(db, "SELECT 'lol'").value();
```
Note, how we use `std::string` instead of `std::string_view`, as a prepared statement is created and then freed internally. You could also use more boiler-plate, if you want to use prepared statements. Most wrapped functions are named similarly to plain-vanilla sqlite functions, with the exception of utility function calls. These leverage various C++-standard provided utility classes and function to make your work easier. If you need something new, a utility function, or you are missing a wrapping of a certain unwrapped sqlite function, please create an issue.
## Example
```c++
#include <iostream>

#include <vector>

#include "sqliteutils.hpp"

using namespace squ::literals;

int main(int, char*[])
{
  auto const db(squ::open_unique("example.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));

  std::cout << "SELECT 1./3"_squ.execget<double>(db).value() << std::endl;

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

  auto stmt(
    "SELECT NAME,AGE,ADDRESS,SALARY FROM COMPANY"_squ.unique(db)
  );

  squ::foreach_row(stmt,
    [](std::string_view const& name, unsigned const age,
      std::string_view const& address, double const salary) noexcept
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

  // this is a cte example query from sqlite3 docs
  stmt = 
    "WITH RECURSIVE\n"
    "xaxis(x) AS (VALUES(-2.0)UNION ALL SELECT x+0.05 FROM xaxis WHERE x<1.2),"
    "yaxis(y) AS (VALUES(-1.0)UNION ALL SELECT y+0.1 FROM yaxis WHERE y<1.0),"
    "m(iter,cx,cy,x,y)AS("
    "SELECT 0,x,y,0.0,0.0 FROM xaxis,yaxis\n"
    "UNION ALL\n"
    "SELECT iter+1,cx,cy,x*x-y*y+cx,2.0*x*y+cy FROM m\n"
    "WHERE(x*x+y*y)<4.0 AND iter<28"
    "),"
    "m2(iter,cx,cy)AS("
    "SELECT max(iter),cx,cy FROM m GROUP BY cx,cy"
    "),"
    "a(t)AS("
    "SELECT group_concat(substr(' .+*#',1+min(iter/7,4),1),'')"
    "FROM m2 GROUP BY cy"
    ")"
    "SELECT group_concat(rtrim(t),x'0a') FROM a"_squ.unique(db);

  squ::foreach_row(stmt,
    [](std::string_view const& s) noexcept
    {
      std::cout << s << std::endl;
    }
  );

  return 0;
}
```

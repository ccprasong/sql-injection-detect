<img src="https://www.cs.cmu.edu/~jarulraj/images/sqlcheck.jpg" alt="SQLCheck Logo">
-----------------
[![GitHub license](https://img.shields.io/badge/license-apache-blue.svg?style=flat)](https://www.apache.org/licenses/LICENSE-2.0)
[![Build Status](https://travis-ci.org/jarulraj/sqlcheck.svg?branch=master)](https://travis-ci.org/jarulraj/sqlcheck)
[![Coverage Status](https://coveralls.io/repos/github/jarulraj/sqlcheck/badge.svg?branch=master)](https://coveralls.io/github/jarulraj/sqlcheck?branch=master)

## What Is SQLCheck?

SQLCheck automates the detection of common anti-patterns in SQL queries. Such anti-patterns often increase the time taken to run the queries on a database management system. Addressing these anti-patterns will, therefore, help improve the runtime performance of the queries. This tool targets all SQL dialects.   

## Installation

SQLCheck has the following software dependencies:

- **g++ 4.9+** 
- **cmake** (`apt-get install cmake`) 

Run the following commands to install SQLCheck:

```
./bootstrap
cd build
cmake -DCMAKE_BUILD_TYPE=RELEASE ..
make
```

## What it can do

Right now SQLCheck can detect the following anti-patterns:

### Logical Database Design Anti-Patterns

  * [Multi-Valued Attribute](https://github.com/jarulraj/sqlcheck/blob/master/docs/logical/1001.md)
  * [Recursive Dependency](https://github.com/jarulraj/sqlcheck/blob/master/docs/logical/1002.md)
  * [Primary Key Does Not Exist](https://github.com/jarulraj/sqlcheck/blob/master/docs/logical/1003.md)
  * [Generic Primary Key](https://github.com/jarulraj/sqlcheck/blob/master/docs/logical/1004.md)
  * [Foreign Key Does Not Exist](https://github.com/jarulraj/sqlcheck/blob/master/docs/logical/1005.md)
  * [Entity-Attribute-Value Pattern](https://github.com/jarulraj/sqlcheck/blob/master/docs/logical/1006.md)
  * [Metadata Tribbles](https://github.com/jarulraj/sqlcheck/blob/master/docs/logical/1007.md)

### Physical Database Design Anti-Patterns

  * [Imprecise Data Type](https://github.com/jarulraj/sqlcheck/blob/master/docs/physical/2001.md)
  * [Values In Definition](https://github.com/jarulraj/sqlcheck/blob/master/docs/physical/2002.md)
  * [Files Are Not SQL Data Types](https://github.com/jarulraj/sqlcheck/blob/master/docs/physical/2003.md)
  * [Too Many Indexes](https://github.com/jarulraj/sqlcheck/blob/master/docs/physical/2004.md)
  * [Index Attribute Order](https://github.com/jarulraj/sqlcheck/blob/master/docs/physical/2005.md)

### Query Anti-Patterns

  * [SELECT *](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3001.md)
  * [NULL Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3002.md)
  * [NOT NULL Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3003.md)
  * [String Concatenation](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3004.md)
  * [GROUP BY Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3005.md)
  * [ORDER BY RAND Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3006.md)
  * [Pattern Matching Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3007.md)
  * [Spaghetti Query Alert](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3008.md)
  * [Reduce Number of JOINs](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3009.md)
  * [Eliminate Unnecessary DISTINCT Conditions](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3010.md)
  * [Implicit Column Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3011.md)
  * [HAVING Clause Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3012.md)
  * [Nested sub queries](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3013.md)
  * [OR Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3014.md)
  * [UNION Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3015.md)
  * [DISTINCT & JOIN Usage](https://github.com/jarulraj/sqlcheck/blob/master/docs/query/3016.md)

### Application Development Anti-Patterns

  * [Readable Passwords](https://github.com/jarulraj/sqlcheck/blob/master/docs/application/4001.md)


# Usage

```
$ ./bin/sqlcheck -h

Command line options : sqlcheck <options>
   -f --file_name          :  file name
   -r --risk_level         :  set of anti-patterns to check
                           :  1 (all anti-patterns, default) 
                           :  2 (only medium and high risk anti-patterns) 
                           :  3 (only high risk anti-patterns) 
   -c --color_mode         :  color mode 
   -v --verbose_mode       :  verbose mode 

$ ./bin/sqlcheck -f sample.sql -v 

-------------------------------------------------
> RISK LEVEL    :: ALL ANTI-PATTERNS
> SQL FILE NAME :: latches/top_mutexes.sql
-------------------------------------------------
==================== Results ===================

-------------------------------------------------
SQL Statement: with top_mutexes as ( select--+ leading(t1 s1 v1 v2 t2 s2) use_hash(s1)
use_nl(v1) use_hash(s2) materialize t1.hsecs ,s1.* ,s2.sleeps as end_sleeps
,s2.wait_time as end_wait_time ,s2.sleeps-s1.sleeps as delta_sleeps ,t2.hsecs -
t1.hsecs as delta_hsecs --,s2.* from v$timer t1 ,v$mutex_sleep s1 ,(select/*+
no_merge */ sum(level) a from dual connect by level<=1e6) v1 ,v$timer t2
,v$mutex_sleep s2 where s1.mutex_type=s2.mutex_type and s1.location=s2.location
) select * from top_mutexes order by delta_sleeps desc;

[latches/top_mutexes.sql]: (HIGH RISK) SELECT *   
● Inefficiency in moving data to the consumer:   
When you SELECT *, you're often retrieving more columns from the database than
your application really needs to function. This causes more data to move from
the database server to the client, slowing access and increasing load on your
machines, as well as taking more time to travel across the network. This is
especially true when someone adds new columns to underlying tables that didn't
exist and weren't needed when the original consumers coded their data access.   


● Indexing issues:   
Consider a scenario where you want to tune a query to a high level of
performance. If you were to use *, and it returned more columns than you
actually needed, the server would often have to perform more expensive methods
to retrieve your data than it otherwise might. For example, you wouldn't be able
to create an index which simply covered the columns in your SELECT list, and
even if you did (including all columns [shudder]), the next guy who came around
and added a column to the underlying table would cause the optimizer to ignore
your optimized covering index, and you'd likely find that the performance of
your query would drop substantially for no readily apparent reason.   

● Binding Problems:   
When you SELECT *, it's possible to retrieve two columns of the same name from
two different tables. This can often crash your data consumer. Imagine a query
that joins two tables, both of which contain a column called "ID". How would a
consumer know which was which? SELECT * can also confuse views (at least in some
versions SQL Server) when underlying table structures change -- the view is not
rebuilt, and the data which comes back can be nonsense. And the worst part of it
is that you can take care to name your columns whatever you want, but the next
guy who comes along might have no way of knowing that he has to worry about
adding a column which will collide with your already-developed names.   
[Matching Expression: select *]

[latches/top_mutexes.sql]: (LOW RISK) Spaghetti Query Alert   
● Split up a complex spaghetti query into several simpler queries:   
SQL is a very expressive language—you can accomplish a lot in a single query
or statement. But that doesn't mean it's mandatory or even a good idea to
approach every task with the assumption it has to be done in one line of code.
One common unintended consequence of producing all your results in one query is
a Cartesian product. This happens when two of the tables in the query have no
condition restricting their relationship. Without such a restriction, the join
of two tables pairs each row in the first table to every row in the other table.
Each such pairing becomes a row of the result set, and you end up with many more
rows than you expect. It's important to consider that these queries are simply
hard to write, hard to modify, and hard to debug. You should expect to get
regular requests for incremental enhancements to your database applications.
Managers want more complex reports and more fields in a user interface. If you
design intricate, monolithic SQL queries, it's more costly and time-consuming to
make enhancements to them. Your time is worth something, both to you and to your
project. Split up a complex spaghetti query into several simpler queries. When
you split up a complex SQL query, the result may be many similar queries,
perhaps varying slightly depending on data values. Writing these queries is a
chore, so it's a good application of SQL code generation. Although SQL makes it
seem possible to solve a complex problem in a single line of code, don't be
tempted to build a house of cards.   

==================== Summary ===================   
All Anti-Patterns  :: 2   
>  High Risk   :: 1   
>  Medium Risk :: 0   
>  Low Risk    :: 1   

```

## References

(1) SQL Anti-patterns: Avoiding the Pitfalls of Database Programming, Bill Karwin  
(2) [Common SQL Anti-patterns](https://stackoverflow.com/questions/346659/what-are-the-most-common-sql-anti-patterns), StackOverflow

## Contributions

Contributions to SQLCheck are always welcome. You can contribute in different ways:
* Open an issue with suggestions for improvements and errors you're facing;
* Fork this repository and submit a pull request;
* Improve the documentation.

## License

Licensed under the [Apache License](LICENSE).

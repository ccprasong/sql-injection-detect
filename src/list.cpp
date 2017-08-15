// LIST SOURCE

#include <regex>

#include "include/list.h"
#include "include/checker.h"

namespace sqlcheck {

// UTILITY

std::string GetTableName(const std::string& sql_statement){
  std::string table_template = "create table";
  std::size_t found = sql_statement.find(table_template);
  if (found == std::string::npos) {
    return "";
  }

  // Locate table name
  auto rest = sql_statement.substr(found + table_template.size());
  // Strip space at beginning
  rest = std::regex_replace(rest, std::regex("^ +| +$|( ) +"), "$1");
  auto table_name = rest.substr(0, rest.find(' '));

  return table_name;
}

bool IsDDLStatement(const std::string& sql_statement){
  std::string create_table_template = "create table";
  std::size_t found = sql_statement.find(create_table_template);
  if (found != std::string::npos) {
    return true;
  }

  std::string alter_table_template = "alter table";
  found = sql_statement.find(alter_table_template);
  if (found != std::string::npos) {
    return true;
  }

  return false;
}

bool IsCreateStatement(const std::string& sql_statement){
  std::string create_table_template = "create table";
  std::size_t found = sql_statement.find(create_table_template);
  if (found != std::string::npos) {
    return true;
  }

  return false;
}

// LOGICAL DATABASE DESIGN


void CheckMultiValuedAttribute(const Configuration& state,
                               const std::string& sql_statement,
                               bool& print_statement){

  std::regex pattern("(id\\s+varchar)|(id\\s+text)|(id\\s+regexp)");
  std::string title = "Multi-Valued Attribute";
  PatternType pattern_type = PatternType::PATTERN_TYPE_LOGICAL_DATABASE_DESIGN;

  auto message =
      "● Store each value in its own column and row:\n"
      "Storing a list of IDs as a VARCHAR/TEXT column can cause performance and data integrity\n"
      "problems. Querying against such a column would require using pattern-matching\n"
      "expressions. It is awkward and costly to join a comma-separated list to matching rows.\n"
      "This will make it harder to validate IDs. Think about what is the greatest number of\n"
      "entries this list must support? Instead of using a multi-valued attribute,\n"
      "consider storing it in a separate table, so that each individual value of that attribute\n"
      "occupies a separate row. Such an intersection table implements a many-to-many relationship\n"
      "between the two referenced tables. This will greatly simplify querying and validating\n"
      "the IDs.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_ERROR,
               pattern_type,
               title,
               message,
               true);

}

void CheckRecursiveDependency(const Configuration& state,
                              const std::string& sql_statement,
                              bool& print_statement){

  std::string table_name = GetTableName(sql_statement);
  if(table_name.empty()){
    return;
  }

  std::regex pattern("(references\\s+" + table_name+ ")");
  std::string title = "Recursive Dependency";
  PatternType pattern_type = PatternType::PATTERN_TYPE_LOGICAL_DATABASE_DESIGN;

  auto message =
      "● Avoid recursive relationships:\n"
      "It’s common for data to have recursive relationships. Data may be organized in a\n"
      "treelike or hierarchical way. However, creating a foreign key constraint to enforce\n"
      "the relationship between two columns in the same table lends to awkward querying.\n"
      "Each level of the tree corresponds to another join. You will need to issue recursive\n"
      "queries to get all descendants or all ancestors of a node.\n"
      "A solution is to construct an additional closure table. It involves storing all paths\n"
      "through the tree, not just those with a direct parent-child relationship.\n"
      "You might want to compare different hierarchical data designs -- closure table,\n"
      "path enumeration, nested sets -- and pick one based on your application's needs.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_ERROR,
               pattern_type,
               title,
               message,
               true);

}

void CheckPrimaryKeyExists(const Configuration& state,
                           const std::string& sql_statement,
                           bool& print_statement){

  auto create_statement = IsCreateStatement(sql_statement);
  if(create_statement == false){
    return;
  }

  std::regex pattern("(primary key)");
  std::string title = "Primary Key Does Not Exist";
  PatternType pattern_type = PatternType::PATTERN_TYPE_LOGICAL_DATABASE_DESIGN;

  auto message =
      "● Consider adding a primary key:\n"
      "A primary key constraint is important when you need to do the following:\n"
      "prevent a table from containing duplicate rows,\n"
      "reference individual rows in queries, and\n"
      "support foreign key references\n"
      "If you don’t use primary key constraints, you create a chore for yourself:\n"
      "checking for duplicate rows. More often than not, you will need to define\n"
      "a primary key for every table. Use compound keys when they are appropriate.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_WARN,
               pattern_type,
               title,
               message,
               false);

}

void CheckGenericPrimaryKey(const Configuration& state,
                            const std::string& sql_statement,
                            bool& print_statement){

  auto ddl_statement = IsDDLStatement(sql_statement);
  if(ddl_statement == false){
    return;
  }

  std::regex pattern("(\\s+[\\(]?id\\s+)|(,id\\s+)|(\\s+id\\s+serial)");
  std::string title = "Generic Primary Key";
  PatternType pattern_type = PatternType::PATTERN_TYPE_LOGICAL_DATABASE_DESIGN;

  auto message =
      "● Skip using a generic primary key (id):\n"
      "Adding an id column to every table causes several effects that make its\n"
      "use seem arbitrary. You might end up creating a redundant key or allow\n"
      "duplicate rows if you add this column in a compound key.\n"
      "The name id is so generic that it holds no meaning. This is especially\n"
      "important when you join two tables and they have the same primary\n"
      "key column name.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_ERROR,
               pattern_type,
               title,
               message,
               true);

}

void CheckForeignKeyExists(const Configuration& state,
                           const std::string& sql_statement,
                           bool& print_statement){

  auto create_statement = IsCreateStatement(sql_statement);
  if(create_statement == false){
    return;
  }

  std::regex pattern("(foreign key)");
  std::string title = "Foreign Key Does Not Exist";
  PatternType pattern_type = PatternType::PATTERN_TYPE_LOGICAL_DATABASE_DESIGN;

  auto message =
      "● Consider adding a foreign key:\n"
      "Are you leaving out the application constraints? Even though it seems at\n"
      "first that skipping foreign key constraints makes your database design\n"
      "simpler, more flexible, or speedier, you pay for this in other ways.\n"
      "It becomes your responsibility to write code to ensure referential integrity\n"
      "manually. Use foreign key constraints to enforce referential integrity.\n"
      "Foreign keys have another feature you can’t mimic using application code:\n"
      "cascading updates to multiple tables. This feature allows you to\n"
      "update or delete the parent row and lets the database takes care of any child\n"
      "rows that reference it. The way you declare the ON UPDATE or ON DELETE clauses\n"
      "in the foreign key constraint allow you to control the result of a cascading\n"
      "operation. Make your database mistake-proof with constraints.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_WARN,
               pattern_type,
               title,
               message,
               false);

}

void CheckVariableAttribute(const Configuration& state,
                            const std::string& sql_statement,
                            bool& print_statement){

  std::string table_name = GetTableName(sql_statement);
  if(table_name.empty()){
    return;
  }

  auto found = table_name.find("attribute");
  if (found == std::string::npos) {
    return;
  }

  std::regex pattern("(attribute)");
  std::string title = "Entity-Attribute-Value Pattern";
  PatternType pattern_type = PatternType::PATTERN_TYPE_LOGICAL_DATABASE_DESIGN;

  auto message =
      "● Dynamic schema with variable attributes:\n"
      "Are you trying to create a schema where you can define new attributes\n"
      "at runtime.? This involves storing attributes as rows in an attribute table.\n"
      "This is referred to as the Entity-Attribute-Value or schemaless pattern.\n"
      "When you use this pattern,  you sacrifice many advantages that a conventional\n"
      "database design would have given you. You can't make mandatory attributes.\n"
      "You can't enforce referential integrity. You might find that attributes are\n"
      "not being named consistently. A solution is to store all related types in one table,\n"
      "with distinct columns for every attribute that exists in any type\n"
      "(Single Table Inheritance). Use one attribute to define the subtype of a given row.\n"
      "Many attributes are subtype-specific, and these columns must\n"
      "be given a null value on any row storing an object for which the attribute\n"
      "does not apply; the columns with non-null values become sparse.\n"
      "Another solution is to create a separate table for each subtype\n"
      "(Concrete Table Inheritance). A third solution mimics inheritance,\n"
      "as though tables were object-oriented classes (Class Table Inheritance).\n"
      "Create a single table for the base type, containing attributes common to\n"
      "all subtypes. Then for each subtype, create another table, with a primary key\n"
      "that also serves as a foreign key to the base table.\n"
      "If you have many subtypes or if you must support new attributes frequently,\n"
      "you can add a BLOB column to store data in a format such as XML or JSON,\n"
      "which encodes both the attribute names and their values.\n"
      "This design is best when you can’t limit yourself to a finite set of subtypes\n"
      "and when you need complete flexibility to define new attributes at any time.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_WARN,
               pattern_type,
               title,
               message,
               true);

}

void CheckMetadataTribbles(const Configuration& state,
                           const std::string& sql_statement,
                           bool& print_statement){

  auto ddl_statement = IsDDLStatement(sql_statement);
  if(ddl_statement == false){
    return;
  }

  std::regex pattern("[A-za-z\\-_@]+[0-9]+ ");
  std::string title = "Metadata Tribbles";
  PatternType pattern_type = PatternType::PATTERN_TYPE_LOGICAL_DATABASE_DESIGN;

  std::string message1 =
      "● Store each value with the same meaning in a single column:\n"
      "Creating multiple columns in a table indicates that you are trying to store\n"
      "a multivalued attribute. This design makes it hard to add or remove values,\n"
      "to ensure the uniqueness of values, and handling growing sets of values.\n"
      "The best solution is to create a dependent table with one column for the\n"
      "multivalue attribute. Store the multiple values in multiple rows instead of\n"
      "multiple columns. Also, define a foreign key in the dependent table to associate\n"
      "the values to its parent row.\n";

  std::string message2 =
      "● Breaking down a table or column by year:\n"
      "You might be trying to split a single column into multiple columns,\n"
      "using column names based on distinct values in another attribute.\n"
      "Each year, you will need to add one more column or table.\n"
      "You are mixing metadata with data. You will now need to make sure that\n"
      "the primary key values are unique across all the split columns or tables.\n"
      "The solution is to use a feature called sharding or horizontal partitioning.\n"
      "(PARTITION BY HASH ( YEAR(...) ). With this feature, you can gain the\n"
      "benefits of splitting a large table without the drawbacks.\n"
      "Partitioning is not defined in the SQL standard, so each brand of database\n"
      "implements it in their own nonstandard way.\n"
      "Another remedy for metadata tribbles is to create a dependent table.\n"
      "Instead of one row per entity with multiple columns for each year,\n"
      "use multiple rows. Don't let data spawn metadata.\n";

  auto message = message1 + "\n" + message2;

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_ERROR,
               pattern_type,
               title,
               message,
               true);

}

// PHYSICAL DATABASE DESIGN

void CheckFloat(const Configuration& state,
                const std::string& sql_statement,
                bool& print_statement){

  std::regex pattern("(float)|(real)|(double precision)|(0\\.000[0-9]*)");
  std::string title = "Imprecise Data Type";
  PatternType pattern_type = PatternType::PATTERN_TYPE_PHYSICAL_DATABASE_DESIGN;

  auto message =
      "● Use precise data types:\n"
      "Virtually any use of FLOAT, REAL, or DOUBLE PRECISION data types is suspect.\n"
      "Most applications that use floating-point numbers don't require the range of\n"
      "values supported by IEEE 754 formats. The cumulative impact of inexact \n"
      "floating-point numbers is severe when calculating aggregates.\n"
      "Instead of FLOAT or its siblings, use the NUMERIC or DECIMAL SQL data types\n"
      "for fixed-precision fractional numbers. These data types store numeric values\n"
      "exactly, up to the precision you specify in the column definition.\n"
      "Do not use FLOAT if you can avoid it.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_ERROR,
               pattern_type,
               title,
               message,
               true);

}

void CheckValuesInDefinition(const Configuration& state,
                             const std::string& sql_statement,
                             bool& print_statement){

  auto ddl_statement = IsDDLStatement(sql_statement);
  if(ddl_statement == false){
    return;
  }

  std::regex pattern("(enum)|(in \\()");
  std::string title = "Values In Definition";
  PatternType pattern_type = PatternType::PATTERN_TYPE_PHYSICAL_DATABASE_DESIGN;

  auto message =
      "● Don't specify values in column definition:\n"
      "With enum, you declare the values as strings,\n"
      "but internally the column is stored as the ordinal number of the string\n"
      "in the enumerated list. The storage is therefore compact, but when you\n"
      "sort a query by this column, the result is ordered by the ordinal value,\n"
      "not alphabetically by the string value. You may not expect this behavior.\n"
      "There's no syntax to add or remove a value from an ENUM or check constraint;\n"
      "you can only redefine the column with a new set of values.\n"
      "Moreover, if you make a value obsolete, you could upset historical data.\n"
      "As a matter of policy, changing metadata — that is, changing the definition\n"
      "of tables and columns—should be infrequent and with attention to testing and\n"
      "quality assurance. There's a better solution to restrict values in a column:\n"
      "create a lookup table with one row for each value you allow.\n"
      "Then declare a foreign key constraint on the old table referencing\n"
      "the new table.\n"
      "Use metadata when validating against a fixed set of values.\n"
      "Use data when validating against a fluid set of values.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_WARN,
               pattern_type,
               title,
               message,
               true);

}

void CheckExternalFiles(const Configuration& state,
                        const std::string& sql_statement,
                        bool& print_statement){

  std::regex pattern("(path varchar)|(unlink\\s?\\()");
  std::string title = "Files Are Not SQL Data Types";
  PatternType pattern_type = PatternType::PATTERN_TYPE_PHYSICAL_DATABASE_DESIGN;

  auto message =
      "● Resources outside the database are not managed by the database:\n"
      "It's common for programmers to be unequivocal that we should always\n"
      "store files external to the database.\n"
      "Files don't obey DELETE, transaction isolation, rollback, or work well with\n"
      "database backup tools. They do not obey SQL access privileges and are not SQL\n"
      "data types.\n"
      "Resources outside the database are not managed by the database.\n"
      "You should consider storing blobs inside the database instead of in\n"
      "external files. You can save the contents of a BLOB column to a file.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_WARN,
               pattern_type,
               title,
               message,
               true);

}

void CheckIndexCount(const Configuration& state,
                     const std::string& sql_statement,
                     bool& print_statement){

  auto create_statement = IsCreateStatement(sql_statement);
  if(create_statement == false){
    return;
  }

  std::size_t min_count = 3;
  std::regex pattern("(index)");
  std::string title = "Too Many Indexes";
  PatternType pattern_type = PatternType::PATTERN_TYPE_PHYSICAL_DATABASE_DESIGN;

  auto message =
      "● Don't create too many indexes:\n"
      "You benefit from an index only if you run queries that use that index.\n"
      "There's no benefit to creating indexes that you don't use.\n"
      "If you cover a database table with indexes, you incur a lot of overhead\n"
      "with no assurance of payoff.\n"
      "Consider dropping unnecessary indexes.\n"
      "If an index provides all the columns we need, then we don't need to read\n"
      "rows of data from the table at all. Consider using such covering indexes.\n"
      "Know your data, know your queries, and maintain the right set of indexes.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_WARN,
               pattern_type,
               title,
               message,
               true,
               min_count);

}

void CheckIndexAttributeOrder(const Configuration& state,
                              const std::string& sql_statement,
                              bool& print_statement){


  std::regex pattern("(create index)");
  std::string title = "Index Attribute Order";
  PatternType pattern_type = PatternType::PATTERN_TYPE_PHYSICAL_DATABASE_DESIGN;

  auto message =
      "● Don't create too many indexes:\n"
      "If you create a compound index for the columns, make sure that the query\n"
      "attributes are in the same order as the index attributes, so that the DBMS\n"
      "can use the index while processing the query.\n"
      "If the query and index attribute orders are not aligned, then the DBMS might\n"
      "be unable to use the index during query processing.\n"
      "EX: CREATE INDEX TelephoneBook ON Accounts(last_name, first_name);\n"
      "SELECT * FROM Accounts ORDER BY first_name, last_name;\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}


// QUERY

void CheckSelectStar(const Configuration& state,
                     const std::string& sql_statement,
                     bool& print_statement){

  std::regex pattern("(select\\s+\\*)");
  std::string title = "SELECT *";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  std::string message1 =
      "● Inefficiency in moving data to the consumer:\n"
      "When you SELECT *, you're often retrieving more columns from the database than\n"
      "your application really needs to function. This causes more data to move from\n"
      "the database server to the client, slowing access and increasing load on your\n"
      "machines, as well as taking more time to travel across the network. This is\n"
      "especially true when someone adds new columns to underlying tables that didn't\n"
      "exist and weren't needed when the original consumers coded their data access.\n";

  std::string message2 =
      "● Indexing issues:\n"
      "Consider a scenario where you want to tune a query to a high level of performance.\n"
      "If you were to use *, and it returned more columns than you actually needed,\n"
      "the server would often have to perform more expensive methods to retrieve your\n"
      "data than it otherwise might. For example, you wouldn't be able to create an index\n"
      "which simply covered the columns in your SELECT list, and even if you did\n"
      "(including all columns [shudder]), the next guy who came around and added a column\n"
      "to the underlying table would cause the optimizer to ignore your optimized covering\n"
      "index, and you'd likely find that the performance of your query would drop\n"
      "substantially for no readily apparent reason.\n";

  std::string message3 =
      "● Binding Problems:\n"
      "When you SELECT *, it's possible to retrieve two columns of the same name from two\n"
      "different tables. This can often crash your data consumer. Imagine a query that joins\n"
      "two tables, both of which contain a column called \"ID\". How would a consumer know\n"
      "which was which? SELECT * can also confuse views (at least in some versions SQL Server)\n"
      "when underlying table structures change -- the view is not rebuilt, and the data which\n"
      "comes back can be nonsense. And the worst part of it is that you can take care to name\n"
      "your columns whatever you want, but the next guy who comes along might have no way of\n"
      "knowing that he has to worry about adding a column which will collide with your\n"
      "already-developed names.\n";

  auto message = message1 + "\n" + message2 + "\n" + message3;

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_ERROR,
               pattern_type,
               title,
               message,
               true);

}

void CheckNullUsage(const Configuration& state,
                    const std::string& sql_statement,
                    bool& print_statement) {

  std::regex pattern("(null)");
  std::string title = "NULL Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Use NULL as a Unique Value:\n"
      "NULL is not the same as zero. A number ten greater than an unknown is still an unknown.\n"
      "NULL is not the same as a string of zero length.\n"
      "Combining any string with NULL in standard SQL returns NULL.\n"
      "NULL is not the same as false. Boolean expressions with AND, OR, and NOT also produce\n"
      "results that some people find confusing.\n"
      "When you declare a column as NOT NULL, it should be because it would make no sense\n"
      "for the row to exist without a value in that column.\n"
      "Use null to signify a missing value for any data type.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}

void CheckNotNullUsage(const Configuration& state,
                       const std::string& sql_statement,
                       bool& print_statement) {

  auto create_statement = IsCreateStatement(sql_statement);
  if(create_statement == false){
    return;
  }

  std::regex pattern("(not null)");
  std::string title = "NOT NULL Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Use NOT NULL only if the column cannot have a missing value:\n"
      "When you declare a column as NOT NULL, it should be because it would make no sense\n"
      "for the row to exist without a value in that column.\n"
      "Use null to signify a missing value for any data type.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_WARN,
               pattern_type,
               title,
               message,
               true);

}

void CheckConcatenation(const Configuration& state,
                        const std::string& sql_statement,
                        bool& print_statement) {


  std::regex pattern("\\|\\|");
  std::string title = "String Concatenation";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Use COALESCE for string concatenation of nullable columns:\n"
      "You may need to force a column or expression to be non-null for the sake of\n"
      "simplifying the query logic, but you don't want that value to be stored."
      "Use COALESCE function to construct the concatenated expression so that a\n"
      "null-valued column doesn't make the whole expression become null.\n"
      "EX: SELECT first_name || COALESCE(' ' || middle_initial || ' ', ' ') || last_name\n"
      "AS full_name FROM Accounts;\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}

void CheckGroupByUsage(const Configuration& state,
                       const std::string& sql_statement,
                       bool& print_statement){

  std::regex pattern("(group by)");
  std::string title = "GROUP BY Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Do not reference non-grouped columns:\n"
      "Every column in the select-list of a query must have a single value row\n"
      "per row group. This is called the Single-Value Rule.\n"
      "Columns named in the GROUP BY clause are guaranteed to be exactly one value\n"
      "per group, no matter how many rows the group matches.\n"
      "Most DBMSs report an error if you try to run any query that tries to return\n"
      "a column other than those columns named in the GROUP BY clause or as\n"
      "arguments to aggregate functions.\n"
      "Every expression in the select list must be contained in either an\n"
      "aggregate function or the GROUP BY clause.\n"
      "Follow the single-value rule to avoid ambiguous query results.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);


}

void CheckOrderByRand(const Configuration& state,
                      const std::string& sql_statement,
                      bool& print_statement){

  std::regex pattern("(order by rand\\()");
  std::string title = "ORDER BY RAND Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Sorting by a nondeterministic expression (RAND()) means the sorting cannot benefit from an index:\n"
      "There is no index containing the values returned by the random function.\n"
      "That’s the point of them being ran- dom: they are different and\n"
      "unpredictable each time they're selected. This is a problem for the performance\n"
      "of the query, because using an index is one of the best ways of speeding up\n"
      "sorting. The consequence of not using an index is that the query result set\n"
      "has to be sorted by the database using a slow table scan.\n"
      "One technique that avoids sorting the table is to choose a random value\n"
      "between 1 and the greatest primary key value.\n"
      "Still another technique that avoids problems found in the preceding alternatives\n"
      "is to count the rows in the data set and return a random number between 0 and\n"
      "the count. Then use this number as an offset when querying the data set.\n"
      "Some queries just cannot be optimized; consider taking a different approach.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_WARN,
               pattern_type,
               title,
               message,
               true);

}

void CheckPatternMatching(const Configuration& state,
                          const std::string& sql_statement,
                          bool& print_statement){

  std::regex pattern("(like)|(regexp)|(similar to)");
  std::string title = "Pattern Matching Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Avoid using vanilla pattern matching:\n"
      "The most important disadvantage of pattern-matching operators is that\n"
      "they have poor performance. A second problem of simple pattern-matching using LIKE\n"
      "or regular expressions is that it can find unintended matches.\n"
      "It's best to use a specialized search engine technology like Apache Lucene, instead of SQL.\n"
      "Another alternative is to reduce the recurring cost of search by saving the result.\n"
      "Consider using vendor extensions like FULLTEXT INDEX in MySQL.\n"
      "More broadly, you don't have to use SQL to solve every problem.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_WARN,
               pattern_type,
               title,
               message,
               true);

}

void CheckSpaghettiQuery(const Configuration& state,
                         const std::string& sql_statement,
                         bool& print_statement){

  std::regex true_pattern(".*");
  std::regex false_pattern("pattern must not exist");
  std::regex pattern;

  std::string title = "Spaghetti Query Alert";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;
  std::size_t spaghetti_query_char_count = 500;

  if(sql_statement.size() >= spaghetti_query_char_count){
    pattern = true_pattern;
  }
  else {
    pattern = false_pattern;
  }

  auto message =
      "● Split up a complex spaghetti query into several simpler queries:\n"
      "SQL is a very expressive language—you can accomplish a lot in a single query or statement.\n"
      "But that doesn't mean it's mandatory or even a good idea to approach every task with the\n"
      "assumption it has to be done in one line of code.\n"
      "One common unintended consequence of producing all your results in one query is\n"
      "a Cartesian product. This happens when two of the tables in the query have no condition\n"
      "restricting their relationship. Without such a restriction, the join of two tables pairs\n"
      "each row in the first table to every row in the other table. Each such pairing becomes a\n"
      "row of the result set, and you end up with many more rows than you expect.\n"
      "It's important to consider that these queries are simply hard to write, hard to modify,\n"
      "and hard to debug. You should expect to get regular requests for incremental enhancements\n"
      "to your database applications. Managers want more complex reports and more fields in a\n"
      "user interface. If you design intricate, monolithic SQL queries, it's more costly and\n"
      "time-consuming to make enhancements to them. Your time is worth something, both to you\n"
      "and to your project.\n"
      "Split up a complex spaghetti query into several simpler queries.\n"
      "When you split up a complex SQL query, the result may be many similar queries,\n"
      "perhaps varying slightly depending on data values. Writing these queries is a chore,\n"
      "so it's a good application of SQL code generation."
      "Although SQL makes it seem possible to solve a complex problem in a single line of code,\n"
      "don't be tempted to build a house of cards.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}

void CheckJoinCount(const Configuration& state,
                    const std::string& sql_statement,
                    bool& print_statement){

  std::regex pattern("(join)");
  std::string title = "Reduce Number of JOINs";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;
  std::size_t min_count = 5;

  auto message =
      "● Reduce Number of JOINs:\n"
      "Too many JOINs is a symptom of complex spaghetti queries. Consider splitting\n"
      "up the complex query into many simpler queries, and reduce the number of JOINs\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true,
               min_count);

}

void CheckDistinctCount(const Configuration& state,
                        const std::string& sql_statement,
                        bool& print_statement){

  std::regex pattern("(distinct)");
  std::string title = "Eliminate Unnecessary DISTINCT Conditions";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;
  std::size_t min_count = 5;

  auto message =
      "● Eliminate Unnecessary DISTINCT Conditions:\n"
      "Too many DISTINCT conditions is a symptom of complex spaghetti queries.\n"
      "Consider splitting up the complex query into many simpler queries,\n"
      "and reduce the number of DISTINCT conditions\n"
      "It is possible that the DISTINCT condition has no effect if a primary key\n"
      "column is part of the result set of columns\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true,
               min_count);

}

void CheckImplicitColumns(const Configuration& state,
                          const std::string& sql_statement,
                          bool& print_statement){

  std::regex pattern("(insert into \\S+ values)");
  std::string title = "Implicit Column Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Explicitly name columns:\n"
      "Although using wildcards and unnamed columns satisfies the goal\n"
      "of less typing, this habit creates several hazards.\n"
      "This can break application refactoring and can harm performance.\n"
      "Always spell out all the columns you need, instead of relying on\n"
      "wild-cards or implicit column lists.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}

void CheckHaving(const Configuration& state,
                 const std::string& sql_statement,
                 bool& print_statement){

  std::regex pattern("(having)");
  std::string title = "HAVING Clause Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Consider removing the HAVING clause:\n"
      "Rewriting the query's HAVING clause into a predicate will enable the\n"
      "use of indexes during query processing.\n"
      "EX: SELECT s.cust_id,count(s.cust_id) FROM SH.sales s GROUP BY s.cust_id\n"
      "HAVING s.cust_id != '1660' AND s.cust_id != '2'; can be rewritten as:\n"
      "SELECT s.cust_id,count(cust_id) FROM SH.sales s WHERE s.cust_id != '1660'\n"
      "AND s.cust_id !='2' GROUP BY s.cust_id;\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}

void CheckNesting(const Configuration& state,
                  const std::string& sql_statement,
                  bool& print_statement){

  std::regex pattern("(select)");
  std::string title = "Nested sub queries";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;
  std::size_t min_count = 2;

  auto message =
      "● Un-nest sub queries:\n"
      " Rewriting nested queries as joins often leads to more efficient\n"
      "execution and more effective optimization. In general, sub-query unnesting\n"
      "is always done for correlated sub-queries with, at most, one table in\n"
      "the FROM clause, which are used in ANY, ALL, and EXISTS predicates.\n"
      "A uncorrelated sub-query, or a sub-query with more than one table in\n"
      "the FROM clause, is flattened if it can be decided, based on the query\n"
      "semantics, that the sub-query returns at most one row.\n"
      "EX: SELECT * FROM SH.products p WHERE p.prod_id = (SELECT s.prod_id FROM SH.sales\n"
      "s WHERE s.cust_id = 100996 AND s.quantity_sold = 1 );\n can be rewritten as:\n"
      "SELECT p.* FROM SH.products p, sales s WHERE p.prod_id = s.prod_id AND\n"
      "s.cust_id = 100996 AND s.quantity_sold = 1;\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true,
               min_count);


}

void CheckOr(const Configuration& state,
                 const std::string& sql_statement,
                 bool& print_statement){

  std::regex pattern("(or)");
  std::string title = "OR Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Consider using an IN predicate when querying an indexed column:\n"
      "The IN-list predicate can be exploited for indexed retrieval and also,\n"
      "the optimizer can sort the IN-list to match the sort sequence of the index,\n"
      "leading to more efficient retrieval. Note that the IN-list must contain only\n"
      "constants, or values that are constant during one execution of the query block,\n"
      "such as outer references.\n"
      "EX: SELECT s.* FROM SH.sales s WHERE s.prod_id = 14 OR s.prod_id = 17;\n"
      "can be rewritten as:\n"
      "SELECT s.* FROM SH.sales s WHERE s.prod_id IN (14, 17);\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}

void CheckUnion(const Configuration& state,
                const std::string& sql_statement,
                bool& print_statement){

  std::regex pattern("(union)");
  std::string title = "UNION Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Consider using UNION ALL if you do not care about duplicates:\n"
      "Unlike UNION which removes duplicates, UNION ALL allows duplicate tuples.\n"
      "If you do not care about duplicate tuples, then using UNION ALL would be\n"
      "a faster option.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}

void CheckDistinctJoin(const Configuration& state,
                       const std::string& sql_statement,
                       bool& print_statement){

  std::regex pattern("(distinct.*join)");
  std::string title = "DISTINCT & JOIN Usage";
  PatternType pattern_type = PatternType::PATTERN_TYPE_QUERY;

  auto message =
      "● Consider using a sub-query with EXISTS instead of DISTINCT:\n"
      "The DISTINCT keyword removes duplicates after sorting the tuples\n."
      "Instead, consider using a sub query with the EXISTS keyword, you can avoid\n"
      "having to return an entire table.\n"
      "EX: SELECT DISTINCT c.country_id, c.country_name FROM SH.countries c,\n"
      "SH.customers e WHERE e.country_id = c.country_id;\n"
      "can be rewritten to:\n"
      "SELECT c.country_id, c.country_name FROM SH.countries c WHERE  EXISTS\n"
      "(SELECT 'X' FROM  SH.customers e WHERE e.country_id = c.country_id);\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}



// APPLICATION

void CheckReadablePasswords(const Configuration& state,
                            const std::string& sql_statement,
                            bool& print_statement){

  std::regex pattern("(password varchar)|(password text)|(password =)|"
      "(pwd varchar)|(pwd text)|(pwd =)");
  std::string title = "Readable Passwords";
  PatternType pattern_type = PatternType::PATTERN_TYPE_APPLICATION;

  auto message =
      "● Do not store readable passwords:\n"
      "It’s not secure to store a password in clear text or even to pass it over the\n"
      "network in the clear. If an attacker can read the SQL statement you use to\n"
      "insert a password, they can see the password plainly.\n"
      "Additionally, interpolating the user's input string into the SQL query in plain text\n"
      "exposes it to discovery by an attacker.\n"
      "If you can read passwords, so can a hacker.\n"
      "The solution is to encode the password using a one-way cryptographic hash \n"
      "function. This function transforms its input string into a new string,\n"
      "called the hash, that is unrecognizable.\n"
      "Use a salt to thwart dictionary attacks. Don't put the plain-text password\n"
      "into the SQL query. Instead, compute the hash in your application code,\n"
      "and use only the hash in the SQL query.\n";

  CheckPattern(state,
               sql_statement,
               print_statement,
               pattern,
               LOG_LEVEL_INFO,
               pattern_type,
               title,
               message,
               true);

}

}  // namespace machine

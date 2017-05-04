// CHECKER HEADER

#pragma once

#include <regex>

#include "configuration.h"

namespace sqlcheck {

// Check a set of SQL statements
void Check(Configuration& state);

// Check a SQL statement
void CheckStatement(const Configuration& state,
                    const std::string& sql_statement);

// Check a pattern
void CheckPattern(const Configuration& state,
                  const std::string& sql_statement,
                  const std::regex& anti_pattern,
                  const LogLevel pattern_level,
                  const PatternType pattern_type,
                  const std::string title,
                  const std::string message);

}  // namespace machine

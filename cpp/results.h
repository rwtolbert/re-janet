#pragma once

#include <janet.h>
#include <string>
#include <vector>

struct ReMatch
{
  int64_t              index  = 0;
  int64_t              begin  = -1;
  int64_t              end    = -1;
  std::string          val    = "";
  std::vector<ReMatch> groups = {};
};

Janet MatchResultsToArray(const std::vector<ReMatch>& matches);

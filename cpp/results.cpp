#include "results.h"

#include <iostream>

Janet
MatchResultsToArray(const std::vector<ReMatch>& matches)
{
  JanetArray* array = janet_array(matches.size());

  for (auto&& m : matches)
  {
    JanetTable* match = janet_table(0);
    janet_table_put(match, janet_ckeywordv("begin"), janet_wrap_integer((int32_t)m.begin));
    janet_table_put(match, janet_ckeywordv("end"), janet_wrap_integer((int32_t)m.end));
    janet_table_put(match, janet_ckeywordv("val"),
                    janet_wrap_string(janet_string((uint8_t*)m.val.data(), m.val.size())));
    if (!m.groups.empty())
    {
      JanetArray* groups = janet_array(m.groups.size());
      for (auto&& g : m.groups)
      {
        JanetTable* group = janet_table(0);
        janet_table_put(group, janet_ckeywordv("group-index"), janet_wrap_integer((int32_t)g.index));
        janet_table_put(group, janet_ckeywordv("begin"), janet_wrap_integer((int32_t)g.begin));
        janet_table_put(group, janet_ckeywordv("end"), janet_wrap_integer((int32_t)g.end));
        janet_table_put(group, janet_ckeywordv("val"),
                        janet_wrap_string(janet_string((uint8_t*)g.val.data(), g.val.size())));
        janet_array_push(groups, janet_wrap_table(group));
      }
      janet_table_put(match, janet_ckeywordv("groups"), janet_wrap_array(groups));
    }
    janet_array_push(array, janet_wrap_table(match));
  }

  return janet_wrap_array(array);
}

#include "wrap_std_regex.h"
#include "results.h"

#include <iostream>
#include <sstream>

namespace
{
const char* ecmascript = "ecmascript";
const char* ignorecase = "ignorecase";
const char* optimize   = "optimize";
const char* collate    = "collate";
const char* basic      = "basic";
const char* extended   = "extended";
const char* awk        = "awk";
const char* grep       = "grep";
const char* egrep      = "egrep";

std::regex::flag_type
get_std_flag_type(JanetKeyword kw)
{
  if (kw == janet_ckeyword(ignorecase))
  {
    return std::regex::icase;
  }
  else if (kw == janet_ckeyword(optimize))
  {
    return std::regex::optimize;
  }
  else if (kw == janet_ckeyword(collate))
  {
    return std::regex::collate;
  }
  else if (kw == janet_ckeyword(basic))
  {
    return std::regex::basic;
  }
  else if (kw == janet_ckeyword(extended))
  {
    return std::regex::extended;
  }
  else if (kw == janet_ckeyword(awk))
  {
    return std::regex::awk;
  }
  else if (kw == janet_ckeyword(grep))
  {
    return std::regex::grep;
  }
  else if (kw == janet_ckeyword(egrep))
  {
    return std::regex::egrep;
  }
  return std::regex::nosubs;
}
} // empty namespace

int
set_gc(void* data, size_t len)
{
  (void)len;
  if (data)
  {
    JanetRegex* re = (JanetRegex*)data;
    if (re->re)
    {
      delete (re->re);
      re->re = nullptr;
    }
    if (re->pattern)
    {
      delete (re->pattern);
      re->pattern = nullptr;
    }
    if (re->flags)
    {
      re->flags->clear();
      delete (re->flags);
      re->flags = nullptr;
    }
  }
  return 0;
}

int
set_gcmark(void* data, size_t len)
{
  (void)len;
  janet_mark(janet_wrap_abstract((JanetRegex*)data));
  return 0;
}

void
set_tostring(void* data, JanetBuffer* buffer)
{
  if (data)
  {
    JanetRegex* re = (JanetRegex*)data;
    if (!re->pattern)
    {
      janet_buffer_push_cstring(buffer, "no pattern");
      return;
    }
    janet_buffer_push_cstring(buffer, "pattern: '");
    janet_buffer_push_cstring(buffer, re->pattern->c_str());
    janet_buffer_push_cstring(buffer, "'");
    if (!re->flags->empty())
    {
      janet_buffer_push_cstring(buffer, " flags: (");
      bool first = true;
      for (int32_t i = 0; i < re->flags->size(); ++i)
      {
        if (!first)
          janet_buffer_push_cstring(buffer, " ");
        janet_buffer_push_cstring(buffer, ":");
        janet_buffer_push_cstring(buffer, re->flags->operator[](i).c_str());
        first = false;
      }
      janet_buffer_push_cstring(buffer, ")");
    }
    else
    {
      janet_buffer_push_cstring(buffer, " flags: ()");
    }
  }
}

const JanetAbstractType regex_type = {
  .name     = "std-regex",
  .gc       = set_gc,
  .gcmark   = set_gcmark,
  .tostring = set_tostring,
};

const char* std_regex_allowed = "[:ignorecase :optimize :collate :ecmascript :basic "
                                ":extended :awk :grep :egrep]";

JanetRegex*
new_abstract_regex(const char* input, const Janet* argv, int32_t flag_start, int32_t argc)
{
  std::regex::flag_type flags = std::regex::ECMAScript;

  JanetRegex* regex = (JanetRegex*)janet_abstract(&regex_type, sizeof(JanetRegex));
  regex->re         = nullptr;
  regex->pattern    = nullptr;
  regex->flags      = new std::vector<std::string>();

  for (int32_t i = flag_start; i < argc; ++i)
  {
    if (!janet_checktype(argv[i], JANET_KEYWORD))
    {
      std::ostringstream os;
      os << "C++ std::regex flags must be keyword from " << std_regex_allowed;
      regex->pattern = new std::string(os.str());
      break;
    }

    auto arg = janet_getkeyword(argv, i);
    if (arg)
    {
      auto thisFlag = get_std_flag_type(arg);
      if (thisFlag == std::regex::nosubs)
      {
        std::ostringstream os;
        os << arg << " is not a valid C++ std::regex flag.\n  Flags must be from list " << std_regex_allowed;
        regex->pattern = new std::string(os.str());
        break;
      }
      flags |= thisFlag;
      JanetBuffer temp;
      janet_buffer_init(&temp, 0);
      janet_buffer_push_string(&temp, arg);
      auto str = std::string((const char*)temp.data, temp.count);
      regex->flags->push_back(str);
      janet_buffer_deinit(&temp);
    }
  }

  // if there is a pattern set, it is an error from above
  if (input && !regex->pattern)
  {
    std::regex* re = nullptr;
    try
    {
      if (flags)
      {
        re = new std::regex(input, flags);
      }
      else
      {
        re = new std::regex(input);
      }
      regex->re      = re;
      regex->pattern = new std::string(input);
    }
    catch (const std::regex_error& e)
    {
      std::ostringstream os;
      os << "Pattern: '" << input << "', " << e.what();
      regex->pattern = new std::string(os.str());
    }
  }
  return regex;
}

JanetTable*
extract_table_from_match(const std::string& input, const std::smatch& match)
{
  JanetTable* results = janet_table(5);

  janet_table_put(results, janet_ckeywordv("begin"), janet_wrap_integer(match.position()));
  janet_table_put(results, janet_ckeywordv("end"), janet_wrap_integer(match.position() + match.length()));
  auto&& sub = match[0];
  auto   bgn = std::distance(input.begin(), sub.first);
  auto   end = bgn + sub.length();
  janet_table_put(results, janet_ckeywordv("val"),
                  janet_wrap_string(janet_string((const uint8_t*)sub.str().data(), sub.str().size())));

  if (match.size() > 1)
  {
    JanetArray* groups = janet_array(0);
    for (size_t j = 1; j < match.size(); ++j)
    {
      if (match[j].matched)
      {
        JanetTable* group = janet_table(4);
        auto&&      sub   = match[j];
        auto        bgn   = std::distance(input.begin(), sub.first);
        auto        end   = bgn + sub.length();

        janet_table_put(group, janet_ckeywordv("group-index"), janet_wrap_integer(j));
        janet_table_put(group, janet_ckeywordv("val"),
                        janet_wrap_string(janet_string((const uint8_t*)sub.str().data(), sub.str().size())));
        Janet matched = sub.matched ? janet_wrap_true() : janet_wrap_false();
        janet_table_put(group, janet_ckeywordv("begin"), janet_wrap_integer(bgn));
        janet_table_put(group, janet_ckeywordv("end"), janet_wrap_integer(end));
        janet_array_push(groups, janet_wrap_table(group));
      }
      janet_table_put(results, janet_ckeywordv("groups"), janet_wrap_array(groups));
    }
  }
  return results;
}

JanetArray*
extract_array_from_iterator(const std::string& input, std::sregex_iterator& iter)
{
  JanetArray* results   = janet_array(0);
  auto        searchEnd = std::sregex_iterator();
  while (iter != searchEnd)
  {
    janet_array_push(results, janet_wrap_table(extract_table_from_match(input, *iter)));
    ++iter;
  }
  return results;
}

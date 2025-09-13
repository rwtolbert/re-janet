/*
 * Copyright (c) 2025 Robert W. Tolbert
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <janet.h>

#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "module.h"

#include <iostream>
#include <sstream>

/*****************/
/* C++ Functions */
/*****************/

namespace
{

struct JanetRegex
{
  JanetGCObject             gc;
  std::regex*               re      = nullptr;
  std::string*              pattern = nullptr;
  std::vector<std::string>* flags   = nullptr;
};

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

const char* ecmascript = "ecmascript";
const char* ignorecase = "ignorecase";
const char* optimize   = "optimize";
const char* collate    = "collate";
const char* basic      = "basic";
const char* extended   = "extended";
const char* awk        = "awk";
const char* grep       = "grep";
const char* egrep      = "egrep";

JanetAbstractType regex_type = {};

void
initialize_regex_type()
{
  if (!regex_type.name)
  {
    regex_type.name     = "std-regex";
    regex_type.gc       = set_gc;
    regex_type.gcmark   = set_gcmark;
    regex_type.tostring = set_tostring;
  }
}

const char* std_regex_allowed = "[:ignorecase :optimize :collate :ecmascript :basic "
                                ":extended :awk :grep :egrep]";

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

JanetRegex*
new_abstract_regex(const char* input, const Janet* argv, int32_t flag_start, int32_t argc)
{
  initialize_regex_type();
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
  auto prefix = match.prefix().str();
  auto suffix = match.suffix().str();
  janet_table_put(results, janet_ckeywordv("prefix"),
                  janet_wrap_string(janet_string((const uint8_t*)prefix.data(), prefix.size())));
  janet_table_put(results, janet_ckeywordv("suffix"),
                  janet_wrap_string(janet_string((const uint8_t*)suffix.data(), suffix.size())));

  auto matches = std::vector<Janet>(match.size());
  for (size_t j = 0; j < match.size(); ++j)
  {
    JanetTable* group = janet_table(4);
    auto&&      sub   = match[j];
    auto        bgn   = std::distance(input.begin(), sub.first);
    auto        end   = bgn + sub.length();

    janet_table_put(group, janet_ckeywordv("index"), janet_wrap_integer(j));
    janet_table_put(group, janet_ckeywordv("str"),
                    janet_wrap_string(janet_string((const uint8_t*)sub.str().data(), sub.str().size())));
    Janet matched = sub.matched ? janet_wrap_true() : janet_wrap_false();
    janet_table_put(group, janet_ckeywordv("matched"), matched);
    janet_table_put(group, janet_ckeywordv("begin"), janet_wrap_integer(bgn));
    janet_table_put(group, janet_ckeywordv("end"), janet_wrap_integer(end));
    matches[j] = janet_wrap_table(group);
  }
  janet_table_put(results, janet_ckeywordv("groups"), janet_wrap_tuple(janet_tuple_n(&matches[0], matches.size())));
  return results;
}

} // namespace

JANET_FN(cfun_std_compile, "(jre/_std-compile regex &opt flags)",
         R"(Compile regex for repeated use.

Flags let you control the syntax and contents of the regex
based on C++ std::regex rules. (https://en.cppreference.com/w/cpp/regex/syntax_option_type)

The following options are available:

* :icase - use case-insensitive matching
* :optimize - optimize regex for matching speed
* :collate - character ranges are locale sensitive

Grammar options: (These are mutually exclusive)

* :ecmascript - Modified ECMAScript grammar (https://en.cppreference.com/w/cpp/regex/ecmascript)
* :basic - basic POSIX regex grammar
* :extended - extended POSIX regex grammar
* :awk - POSIX awk regex grammar
* :grep - POSIX grep regex grammar
* :egrep - POSIX egrep regex grammar
)")
{
  janet_arity(argc, 1, 6);
  const char* input = janet_getcstring(argv, 0);
  JanetRegex* regex = new_abstract_regex(input, argv, 1, argc);
  if (regex->re)
    return janet_wrap_abstract(regex);

  if (regex->pattern)
  {
    auto  size   = regex->pattern->size() * sizeof(char);
    char* output = (char*)alloca(size + 1);
    snprintf(output, regex->pattern->size() + 1, "%s", regex->pattern->c_str());
    set_gcmark(regex, 0);
    janet_panic(output);
  }
  else
  {
    set_gcmark(regex, 0);
    janet_panic("Unknown RE compile error.");
  }
  return janet_wrap_nil();
}

JANET_FN(cfun_std_contains, "(jre/_std-contains regex str)",
         R"(Match a pre-compiled regex or regex string to an input string.

Return true if the match regex is in the str.
)")
{
  janet_fixarity(argc, 2);

  JanetRegex* regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_regex(re_string, argv, 0, 0);
  }
  else if (janet_checkabstract(argv[0], &regex_type))
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :std");
  }

  const char* input  = janet_getcstring(argv, 1);
  auto        result = false;
  if (input && regex->re)
  {
    std::string s(input);
    std::smatch m;
    auto        search_begin = std::sregex_iterator(s.begin(), s.end(), *regex->re);
    auto        search_end   = std::sregex_iterator();
    auto        count        = std::distance(search_begin, search_end);
    return janet_wrap_boolean(count > 0);
  }
  return janet_wrap_nil();
}

/*
JANET_FN(cfun_std_match, "(jre/_std-match regex str)",
         R"(Match a pre-compiled regex or regex string to an input string.

Match is successful when the regex matches the entire input string.
To match sub-strings, use `jre/search`.

Results are in a struct.

If you need a regex with options beyond the default, use `jre/compile`
to pre-compile it. Otherwise, you can just pass the regex as a string
and it will be compiled on-the-fly.
)")
{
  janet_fixarity(argc, 2);

  JanetRegex* regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_regex(re_string, argv, 0, 0);
  }
  else
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }

  const char* input  = janet_getcstring(argv, 1);
  auto        result = false;
  if (input && regex->re)
  {
    std::string s(input);
    std::smatch m;
    result = std::regex_match(s, m, *regex->re);
    if (result)
    {
      JanetTable* table = extract_table_from_match(s, m);
      return janet_wrap_table(table);
    }
  }
  return janet_wrap_nil();
}
*/

JANET_FN(cfun_std_find, "(jre/_std-find regex text &opt start-index)",
         R"(Search a pre-compiled regex or regex string inside input text.

Return position of first match. Optionally, start search at `start-index`.
)")
{
  janet_arity(argc, 2, 3);

  bool        localRegex = false;
  JanetRegex* regex      = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_regex(re_string, argv, 0, 0);
    localRegex            = true;
  }
  else
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }

  int startIndex = 0;
  if (argc == 3)
  {
    startIndex = janet_getinteger(argv, 2);
    if (startIndex <= 0)
      startIndex = 0;
  }

  const char* input  = janet_getcstring(argv, 1);
  int         result = -1;
  if (input && regex->re)
  {
    std::string s(input);

    auto searchBegin = std::sregex_iterator(s.begin(), s.end(), *regex->re);
    auto searchEnd   = std::sregex_iterator();

    while (searchBegin != searchEnd)
    {
      if (searchBegin->position() >= startIndex)
      {
        result = searchBegin->position();
        break;
      }
      ++searchBegin;
    }
  }

  // clean up local regex.
  if (localRegex)
    set_gcmark(regex, 0);

  if (result >= 0)
    return janet_wrap_integer(result);
  return janet_wrap_nil();
}

JANET_FN(cfun_std_findall, "(jre/_std-findall regex text &opt start-index)",
         R"(Search a pre-compiled regex or regex string inside input text.

Return positions of all matches, optionally only after `start-index`.
)")
{
  janet_arity(argc, 2, 3);

  bool        localRegex = false;
  JanetRegex* regex      = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_regex(re_string, argv, 0, 0);
    localRegex            = true;
  }
  else
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }

  int startIndex = 0;
  if (argc == 3)
  {
    startIndex = janet_getinteger(argv, 2);
    if (startIndex <= 0)
      startIndex = 0;
  }

  const char* input  = janet_getcstring(argv, 1);
  JanetArray* result = janet_array(0);

  if (input && regex->re)
  {
    std::string s(input);

    auto searchBegin = std::sregex_iterator(s.begin(), s.end(), *regex->re);
    auto searchEnd   = std::sregex_iterator();

    while (searchBegin != searchEnd)
    {
      if (searchBegin->position() >= startIndex)
        janet_array_push(result, janet_wrap_integer((int)searchBegin->position()));
      ++searchBegin;
    }
  }

  // clean up local regex.
  if (localRegex)
    set_gcmark(regex, 0);

  return janet_wrap_array(result);
}

JANET_FN(cfun_std_replace, "(jre/_std-replace regex text subst)",
         R"(Replace the first instance of `regex` inside `text` with `subst`.

If you need a regex with options beyond the default, use `jre/compile`
to pre-compile it. Otherwise, you can just pass the regex as a string
and it will be compiled on-the-fly.
)")
{
  janet_fixarity(argc, 3);
  JanetRegex* regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_regex(re_string, argv, 3, argc);
  }
  else
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }
  const char* input   = janet_getcstring(argv, 1);
  const char* replace = janet_getcstring(argv, 2);
  if (input && replace && regex->re)
  {
    auto result = std::regex_replace(input, *regex->re, replace, std::regex_constants::format_first_only);
    return janet_wrap_string(janet_cstring(result.c_str()));
  }
  return janet_wrap_string(janet_cstring(input));
}

JANET_FN(cfun_std_replace_all, "(jre/_std-replace-all regex text subst)",
         R"(Replace *all* instances of `regex` inside `text` with `subst`.

If you need a regex with options beyond the default, use `jre/compile`
to pre-compile it. Otherwise, you can just pass the regex as a string
and it will be compiled on-the-fly.
)")
{
  janet_fixarity(argc, 3);
  JanetRegex* regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_regex(re_string, argv, 3, argc);
  }
  else
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }
  const char* input   = janet_getcstring(argv, 1);
  const char* replace = janet_getcstring(argv, 2);
  if (input && replace && regex->re)
  {
    auto result = std::regex_replace(input, *regex->re, replace);
    return janet_wrap_string(janet_cstring(result.c_str()));
  }
  return janet_wrap_string(janet_cstring(input));
}

///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// PCRE2
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

namespace
{

const char* pcre2_allowed = "[:ignorecase]";

uint32_t
get_pcre2_flag_type(JanetKeyword kw)
{
  if (kw == janet_ckeyword(ignorecase))
  {
    return PCRE2_CASELESS;
  }
  return 0;
}

struct JanetPCRE2Regex
{
  JanetGCObject             gc;
  pcre2_code*               re      = nullptr;
  std::string*              pattern = nullptr;
  std::vector<std::string>* flags   = nullptr;
  bool                      jit     = false;
};

int
pcre2_set_gc(void* data, size_t len)
{
  (void)len;
  if (data)
  {
    JanetPCRE2Regex* re = (JanetPCRE2Regex*)data;
    if (re->re)
    {
      pcre2_code_free(re->re);
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
pcre2_set_gcmark(void* data, size_t len)
{
  (void)len;
  janet_mark(janet_wrap_abstract((JanetPCRE2Regex*)data));
  return 0;
}

void
pcre2_set_tostring(void* data, JanetBuffer* buffer)
{
  if (data)
  {
    JanetPCRE2Regex* re = (JanetPCRE2Regex*)data;
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

JanetAbstractType pcre2_regex_type = {};

void
initialize_pcre2_regex_type()
{
  if (!pcre2_regex_type.name)
  {
    pcre2_regex_type.name     = "pcre2";
    pcre2_regex_type.gc       = pcre2_set_gc;
    pcre2_regex_type.gcmark   = pcre2_set_gcmark;
    pcre2_regex_type.tostring = pcre2_set_tostring;
  }
}

JanetPCRE2Regex*
new_abstract_pcre2_regex(const char* input, const Janet* argv, int32_t flag_start, int32_t argc)
{
  initialize_pcre2_regex_type();
  JanetPCRE2Regex* regex = (JanetPCRE2Regex*)janet_abstract(&pcre2_regex_type, sizeof(JanetPCRE2Regex));
  regex->re              = nullptr;
  regex->pattern         = nullptr;
  regex->flags           = new std::vector<std::string>();
  regex->jit             = false;
  uint32_t options       = 0;

  for (int32_t i = flag_start; i < argc; ++i)
  {
    if (!janet_checktype(argv[i], JANET_KEYWORD))
    {
      std::ostringstream os;
      os << "PCRE2 regex flags must be keyword from " << pcre2_allowed;
      regex->pattern = new std::string(os.str());
      break;
    }
    auto arg = janet_getkeyword(argv, i);
    if (arg)
    {
      auto ft = get_pcre2_flag_type(arg);
      if (ft == 0)
      {
        std::ostringstream os;
        os << ":" << arg << " is not a valid PCRE2 regex flag.\n  Flags should be from list " << pcre2_allowed;
        regex->pattern = new std::string(os.str());
        break;
      }
      options |= ft;
      JanetBuffer temp;
      janet_buffer_init(&temp, 0);
      janet_buffer_push_string(&temp, arg);
      auto str = std::string((const char*)temp.data, temp.count);
      regex->flags->push_back(str);
      janet_buffer_deinit(&temp);
    }
  }

  int        errornumber;
  PCRE2_SIZE erroroffset;

  // if there is a pattern set, it is an error from above
  if (input && !regex->pattern)
  {
    auto* re = pcre2_compile((PCRE2_SPTR)input,     /* the pattern */
                             PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
                             options,               /* default options */
                             &errornumber,          /* for error number */
                             &erroroffset,          /* for error offset */
                             NULL);                 /* use default compile context */

    if (re == NULL)
    {
      PCRE2_UCHAR buffer[256];
      pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
      std::ostringstream os;
      os << "PCRE2 compilation failed, pattern: '" << input << "', offset " << erroroffset << ": " << buffer << ".";
      regex->pattern = new std::string(os.str());
    }
    else
    {
      regex->re      = re;
      regex->pattern = new std::string(input);
    }
    if (pcre2_jit_compile(regex->re, PCRE2_JIT_COMPLETE) >= 0)
      regex->jit = true;
  }

  return regex;
}
}

JANET_FN(cfun_pcre2_compile, "(jre/pcre2-compile patt flags)", R"(JIT compile patt into PCRE2 regex.)")
{
  janet_arity(argc, 1, 2);
  const char*      input = janet_getcstring(argv, 0);
  JanetPCRE2Regex* regex = new_abstract_pcre2_regex(input, argv, 1, argc);
  if (regex->re)
    return janet_wrap_abstract(regex);
  else
  {
    if (regex->pattern)
    {
      auto  size   = regex->pattern->size() * sizeof(char);
      char* output = (char*)alloca(size + 1);
      snprintf(output, regex->pattern->size() + 1, "%s", regex->pattern->c_str());
      pcre2_set_gcmark(regex, 0);
      janet_panic(output);
    }
    else
    {
      pcre2_set_gcmark(regex, 0);
      janet_panic("Unknown PCRE2 compile error.");
    }
  }
  return janet_wrap_nil();
}

JANET_FN(cfun_pcre2_contains, "(jre/_pcre2-contains regex text)", R"(Quick test for existence of match in text.)")
{
  janet_fixarity(argc, 2);

  JanetPCRE2Regex* regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_pcre2_regex(re_string, argv, 0, 0);
  }
  else if (janet_checkabstract(argv[0], &pcre2_regex_type))
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :pcre2");
  }

  const char* input  = janet_getcstring(argv, 1);
  auto        result = false;

  auto match_data = pcre2_match_data_create_from_pattern(regex->re, NULL);

  int rc;
  if (regex->jit)
  {
    rc = pcre2_jit_match(regex->re,         /* the compiled pattern */
                         (PCRE2_SPTR)input, /* the subject string */
                         strlen(input),     /* the length of the subject */
                         0,                 /* start at offset 0 in the subject */
                         0,                 /* default options */
                         match_data,        /* block for storing the result */
                         NULL);
  }
  else
  {
    rc = pcre2_match(regex->re,         /* the compiled pattern */
                     (PCRE2_SPTR)input, /* the subject string */
                     strlen(input),     /* the length of the subject */
                     0,                 /* start at offset 0 in the subject */
                     0,                 /* default options */
                     match_data,        /* block for storing the result */
                     NULL);
  }

  if (rc > 0)
    result = true;

  pcre2_match_data_free(match_data);
  return janet_wrap_boolean(result);
}

JANET_FN(cfun_pcre2_find, "(jre/_pcre2-find regex text &opt start-index)", R"(Find first index of regex in text.)")
{
  janet_arity(argc, 2, 3);

  auto options = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH;

  bool             localRegex = false; // if regex is created here, we need to clean it up on exit
  JanetPCRE2Regex* regex      = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    // TODO handle failed to compile local regex from string
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_pcre2_regex(re_string, argv, 0, 0);
    localRegex            = true;
  }
  else
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }

  PCRE2_SIZE startIndex = 0;
  if (argc == 3)
  {
    startIndex = janet_getinteger(argv, 2);
    if (startIndex <= 0)
      startIndex = 0;
  }

  const char* input = janet_getcstring(argv, 1);

  auto match_data = pcre2_match_data_create_from_pattern(regex->re, NULL);

  int rc;
  if (regex->jit)
  {
    rc = pcre2_jit_match(regex->re,         /* the compiled pattern */
                         (PCRE2_SPTR)input, /* the subject string */
                         strlen(input),     /* the length of the subject */
                         startIndex,        /* start at offset in the subject */
                         options,           /* default options */
                         match_data,        /* block for storing the result */
                         NULL);
  }
  else
  {
    rc = pcre2_match(regex->re,         /* the compiled pattern */
                     (PCRE2_SPTR)input, /* the subject string */
                     strlen(input),     /* the length of the subject */
                     startIndex,        /* start at offset in the subject */
                     options,           /* default options */
                     match_data,        /* block for storing the result */
                     NULL);
  }

  // handle the case of failed match
  // TODO - propagate error in janet_panic
  if (rc <= 0)
  {
    pcre2_match_data_free(match_data);
    if (localRegex)
      pcre2_set_gcmark(regex, 0);
    return janet_wrap_nil();
  }

  auto ovector = pcre2_get_ovector_pointer(match_data);
  int  result  = (int)ovector[0];
  pcre2_match_data_free(match_data);
  return janet_wrap_integer(result);
}

JANET_FN(cfun_pcre2_findall, "(jre/_pcre2-findall regex text &opt start-index)",
         R"(Find position of all matches of regex in text.)")
{
  janet_arity(argc, 2, 3);

  bool all     = true;
  auto options = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH;
  if (all)
    options |= PCRE2_SUBSTITUTE_GLOBAL;

  // if regex is created here, we need to clean it up on exit
  bool             localRegex = false;
  JanetPCRE2Regex* regex      = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    // TODO handle failed to compile local regex from string
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_pcre2_regex(re_string, argv, 0, 0);
    localRegex            = true;
  }
  else
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }

  PCRE2_SIZE startIndex = 0;
  if (argc == 3)
  {
    startIndex = janet_getinteger(argv, 2);
    if (startIndex <= 0)
      startIndex = 0;
  }

  const char* input = janet_getcstring(argv, 1);

  auto match_data     = pcre2_match_data_create_from_pattern(regex->re, NULL);
  auto subject_length = strlen(input);
  int  rc;
  if (regex->jit)
  {
    rc = pcre2_jit_match(regex->re,         /* the compiled pattern */
                         (PCRE2_SPTR)input, /* the subject string */
                         subject_length,    /* the length of the subject */
                         startIndex,        /* start at offset in the subject */
                         options,           /* default options */
                         match_data,        /* block for storing the result */
                         NULL);
  }
  else
  {
    rc = pcre2_match(regex->re,         /* the compiled pattern */
                     (PCRE2_SPTR)input, /* the subject string */
                     subject_length,    /* the length of the subject */
                     startIndex,        /* start at offset in the subject */
                     options,           /* default options */
                     match_data,        /* block for storing the result */
                     NULL);
  }

  // handle the case of failed match
  // TODO - propagate error in janet_panic
  if (rc <= 0)
  {
    pcre2_match_data_free(match_data);
    if (localRegex)
      pcre2_set_gcmark(regex, 0);
    return janet_wrap_nil();
  }

  auto ovector = pcre2_get_ovector_pointer(match_data);

  JanetArray* array = janet_array(0);
  janet_array_push(array, janet_wrap_integer((int)ovector[0]));

  // now loop over any subsequent matches
  for (;;)
  {
    uint32_t   options      = 0;          /* Normally no options */
    PCRE2_SIZE start_offset = ovector[1]; /* Start at end of previous match */

    /* If the previous match was for an empty string, we are finished if we are
      at the end of the subject. Otherwise, arrange to run another match at the
      same point to see if a non-empty match can be found. */

    if (ovector[0] == ovector[1])
    {
      if (ovector[0] == subject_length)
        break;
      options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
    }
    else
    {
      PCRE2_SIZE startchar = pcre2_get_startchar(match_data);
      if (start_offset <= startchar)
      {
        if (startchar >= subject_length)
          break; /* Reached end of subject.   */

        //   start_offset = startchar + 1; /* Advance by one character. */
        // if (utf8)                     /* If UTF-8, it may be more  */
        // {                             /*   than one code unit.     */
        //   for (; start_offset < subject_length; start_offset++)
        //     if ((subject[start_offset] & 0xc0) != 0x80)
        //       break;
        // }
      }
    }

    /* Run the next matching operation */
    if (regex->jit)
    {
      rc = pcre2_jit_match(regex->re,         /* the compiled pattern */
                           (PCRE2_SPTR)input, /* the subject string */
                           subject_length,    /* the length of the subject */
                           start_offset,      /* start at offset in the subject */
                           options,           /* default options */
                           match_data,        /* block for storing the result */
                           NULL);
    }
    else
    {
      rc = pcre2_match(regex->re,         /* the compiled pattern */
                       (PCRE2_SPTR)input, /* the subject string */
                       subject_length,    /* the length of the subject */
                       start_offset,      /* start at offset in the subject */
                       options,           /* default options */
                       match_data,        /* block for storing the result */
                       NULL);
    }

    /* This time, a result of NOMATCH isn't an error. If the value in "options"
is zero, it just means we have found all possible matches, so the loop ends.
Otherwise, it means we have failed to find a non-empty-string match at a
point where there was a previous empty-string match. In this case, we do what
Perl does: advance the matching position by one character, and continue. We
do this by setting the "end of previous match" offset, because that is picked
up at the top of the loop as the point at which to start again.

There are two complications: (a) When CRLF is a valid newline sequence, and
the current position is just before it, advance by an extra byte. (b)
Otherwise we must ensure that we skip an entire UTF character if we are in
UTF mode. */

    if (rc == PCRE2_ERROR_NOMATCH)
    {
      if (options == 0)
        break; /* All matches found */
      // ovector[1] = start_offset + 1;           /* Advance one code unit */
      // if (crlf_is_newline &&                   /* If CRLF is a newline & */
      //     start_offset < subject_length - 1 && /* we are at CRLF, */
      //     subject[start_offset] == '\r' && subject[start_offset + 1] == '\n')
      // ovector[1] += 1;                    /* Advance by one more. */
      // else if (utf8)                        /* Otherwise, ensure we */
      // {                                     /* advance a whole UTF-8 */
      //   while (ovector[1] < subject_length) /* character. */
      //   {
      //     if ((subject[ovector[1]] & 0xc0) != 0x80)
      //       break;
      //     ovector[1] += 1;
      //   }
      // }
      continue; /* Go round the loop again */
    }

    /* Other matching errors are not recoverable. */
    // TODO - propagate error in janet_panic
    if (rc < 0)
    {
      pcre2_match_data_free(match_data);
      if (localRegex)
        pcre2_set_gcmark(regex, 0);
      return janet_wrap_nil();
    }

    janet_array_push(array, janet_wrap_integer((int)ovector[0]));
  }

  pcre2_match_data_free(match_data);
  return janet_wrap_array(array);
}

JANET_FN(cfun_pcre2_match, "(jre/pcre2-match regex text)", R"(Return array of captured values.)")
{
  // TODO add ability to pass in startOffset
  PCRE2_SIZE startOffset = 0;
  janet_fixarity(argc, 2);

  auto options = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH | PCRE2_SUBSTITUTE_GLOBAL;

  // if regex is created here, we need to clean it up on exit
  bool             localRegex = false;
  JanetPCRE2Regex* regex      = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    // TODO handle failed to compile local regex from string
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_pcre2_regex(re_string, argv, 0, 0);
    localRegex            = true;
  }
  else
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }

  const char* input = janet_getcstring(argv, 1);

  auto match_data     = pcre2_match_data_create_from_pattern(regex->re, NULL);
  auto subject_length = strlen(input);
  int  rc;
  if (regex->jit)
  {
    rc = pcre2_jit_match(regex->re,         /* the compiled pattern */
                         (PCRE2_SPTR)input, /* the subject string */
                         subject_length,    /* the length of the subject */
                         startOffset,       /* start at offset in the subject */
                         options,           /* default options */
                         match_data,        /* block for storing the result */
                         NULL);
  }
  else
  {
    rc = pcre2_match(regex->re,         /* the compiled pattern */
                     (PCRE2_SPTR)input, /* the subject string */
                     subject_length,    /* the length of the subject */
                     startOffset,       /* start at offset in the subject */
                     options,           /* default options */
                     match_data,        /* block for storing the result */
                     NULL);
  }

  // handle the case of failed match
  // TODO - propagate error in janet_panic
  if (rc <= 0)
  {
    pcre2_match_data_free(match_data);
    if (localRegex)
      pcre2_set_gcmark(regex, 0);
    return janet_wrap_nil();
  }

  auto ovector = pcre2_get_ovector_pointer(match_data);

  JanetArray* array = janet_array(0);

  for (int i = 0; i < rc; i++)
  {
    PCRE2_SPTR substring_start  = (PCRE2_SPTR)input + ovector[2 * i];
    PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
    // printf("%2d: %.*s\n", i, (int)substring_length, (char*)substring_start);
    janet_array_push(array, janet_wrap_string(janet_string((uint8_t*)substring_start, substring_length)));
  }

  // now loop over any subsequent matches
  for (;;)
  {
    uint32_t   options      = 0;          /* Normally no options */
    PCRE2_SIZE start_offset = ovector[1]; /* Start at end of previous match */

    /* If the previous match was for an empty string, we are finished if we are
      at the end of the subject. Otherwise, arrange to run another match at the
      same point to see if a non-empty match can be found. */

    if (ovector[0] == ovector[1])
    {
      if (ovector[0] == subject_length)
        break;
      options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
    }
    else
    {
      PCRE2_SIZE startchar = pcre2_get_startchar(match_data);
      if (start_offset <= startchar)
      {
        if (startchar >= subject_length)
          break; /* Reached end of subject.   */

        //   start_offset = startchar + 1; /* Advance by one character. */
        // if (utf8)                     /* If UTF-8, it may be more  */
        // {                             /*   than one code unit.     */
        //   for (; start_offset < subject_length; start_offset++)
        //     if ((subject[start_offset] & 0xc0) != 0x80)
        //       break;
        // }
      }
    }

    /* Run the next matching operation */
    if (regex->jit)
    {
      rc = pcre2_jit_match(regex->re,         /* the compiled pattern */
                           (PCRE2_SPTR)input, /* the subject string */
                           subject_length,    /* the length of the subject */
                           start_offset,      /* start at offset in the subject */
                           options,           /* default options */
                           match_data,        /* block for storing the result */
                           NULL);
    }
    else
    {
      rc = pcre2_match(regex->re,         /* the compiled pattern */
                       (PCRE2_SPTR)input, /* the subject string */
                       subject_length,    /* the length of the subject */
                       start_offset,      /* start at offset in the subject */
                       options,           /* default options */
                       match_data,        /* block for storing the result */
                       NULL);
    }

    /* This time, a result of NOMATCH isn't an error. If the value in "options"
is zero, it just means we have found all possible matches, so the loop ends.
Otherwise, it means we have failed to find a non-empty-string match at a
point where there was a previous empty-string match. In this case, we do what
Perl does: advance the matching position by one character, and continue. We
do this by setting the "end of previous match" offset, because that is picked
up at the top of the loop as the point at which to start again.

There are two complications: (a) When CRLF is a valid newline sequence, and
the current position is just before it, advance by an extra byte. (b)
Otherwise we must ensure that we skip an entire UTF character if we are in
UTF mode. */

    if (rc == PCRE2_ERROR_NOMATCH)
    {
      if (options == 0)
        break; /* All matches found */
      // ovector[1] = start_offset + 1;           /* Advance one code unit */
      // if (crlf_is_newline &&                   /* If CRLF is a newline & */
      //     start_offset < subject_length - 1 && /* we are at CRLF, */
      //     subject[start_offset] == '\r' && subject[start_offset + 1] == '\n')
      // ovector[1] += 1;                    /* Advance by one more. */
      // else if (utf8)                        /* Otherwise, ensure we */
      // {                                     /* advance a whole UTF-8 */
      //   while (ovector[1] < subject_length) /* character. */
      //   {
      //     if ((subject[ovector[1]] & 0xc0) != 0x80)
      //       break;
      //     ovector[1] += 1;
      //   }
      // }
      continue; /* Go round the loop again */
    }

    /* Other matching errors are not recoverable. */
    // TODO - propagate error in janet_panic
    if (rc < 0)
    {
      pcre2_match_data_free(match_data);
      if (localRegex)
        pcre2_set_gcmark(regex, 0);
      return janet_wrap_nil();
    }

    for (int i = 0; i < rc; i++)
    {
      PCRE2_SPTR substring_start  = (PCRE2_SPTR)input + ovector[2 * i];
      PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
      // printf("%2d: %.*s\n", i, (int)substring_length, (char*)substring_start);
      janet_array_push(array, janet_wrap_string(janet_string((uint8_t*)substring_start, substring_length)));
    }
  }

  pcre2_match_data_free(match_data);
  return janet_wrap_array(array);
}

Janet
pcre2_replace_w_options(JanetPCRE2Regex* regex, const char* input, const char* replace, bool all)
{
  if (input && replace && regex->re)
  {
    // need to dynamically size, based on size of matches and number of matches?
    auto match_data = pcre2_match_data_create_from_pattern(regex->re, NULL);
    int  rc;
    if (regex->jit)
    {
      rc = pcre2_jit_match(regex->re,         /* the compiled pattern */
                           (PCRE2_SPTR)input, /* the subject string */
                           strlen(input),     /* the length of the subject */
                           0,                 /* start at offset 0 in the subject */
                           0,                 /* default options */
                           match_data,        /* block for storing the result */
                           NULL);
    }
    else
    {
      rc = pcre2_match(regex->re,         /* the compiled pattern */
                       (PCRE2_SPTR)input, /* the subject string */
                       strlen(input),     /* the length of the subject */
                       0,                 /* start at offset 0 in the subject */
                       0,                 /* default options */
                       match_data,        /* block for storing the result */
                       NULL);
    }

    // if there is no original match, return the input string
    if (rc < 0)
    {
      pcre2_match_data_free(match_data);
      return janet_wrap_string(janet_cstring(input));
    }

    PCRE2_UCHAR output[2048] = "";
    PCRE2_SIZE  outlen       = sizeof(output) / sizeof(PCRE2_UCHAR);

    auto options = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH;
    if (all)
      options |= PCRE2_SUBSTITUTE_GLOBAL;

    rc = pcre2_substitute(regex->re,
                          (PCRE2_SPTR)input,   // input string to replace into
                          strlen(input),       // length of input string
                          0,                   // offset
                          options,             // options
                          match_data,          // match_data
                          NULL,                // mcontext
                          (PCRE2_SPTR)replace, // string to replace matches with
                          strlen(replace),     // length of replacement string
                          output,              // output buffer
                          &outlen);

    PCRE2_UCHAR* out2 = nullptr;
    if (rc == PCRE2_ERROR_NOMEMORY)
    {
      printf("ran out of memory, input: %d needed: %d\n", strlen(input), outlen);
      out2 = (PCRE2_UCHAR*)malloc(outlen * sizeof(PCRE2_UCHAR));
      rc   = pcre2_substitute(regex->re,
                              (PCRE2_SPTR)input,   // input string to replace into
                              strlen(input),       // length of input string
                              0,                   // offset
                              options,             // options
                              match_data,          // match_data
                              NULL,                // mcontext
                              (PCRE2_SPTR)replace, // string to replace matches with
                              strlen(replace),     // length of replacement string
                              out2,                // output buffer
                              &outlen);
    }

    pcre2_match_data_free(match_data);
    if (rc >= 0)
    {
      if (out2)
      {
        auto res = janet_wrap_string(janet_cstring((char*)out2));
        free(out2);
        return res;
      }
      else
      {
        return janet_wrap_string(janet_cstring((char*)output));
      }
    }
  }
  return janet_wrap_string(janet_cstring(input));
}

JANET_FN(cfun_pcre2_replace, "(jre/pcre2-replace regex text subst)",
         R"(Replace first instance of `regex` inside `text` with `subst`.

If you need a regex with options beyond the default, use `jre/pcre2-compile`
to pre-compile it. Otherwise, you can just pass the regex as a string
and it will be compiled on-the-fly.
)")
{
  janet_fixarity(argc, 3);
  JanetPCRE2Regex* regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_pcre2_regex(re_string, argv, 3, argc);
  }
  else
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }
  const char* input   = janet_getcstring(argv, 1);
  const char* replace = janet_getcstring(argv, 2);
  return pcre2_replace_w_options(regex, input, replace, false);
}

JANET_FN(cfun_pcre2_replace_all, "(jre/pcre2-replace-all regex text subst)",
         R"(Replace *all* instances of `regex` inside `text` with `subst`.

If you need a regex with options beyond the default, use `jre/pcre2-compile`
to pre-compile it. Otherwise, you can just pass the regex as a string
and it will be compiled on-the-fly.
)")
{
  janet_fixarity(argc, 3);
  JanetPCRE2Regex* regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING))
  {
    const char* re_string = janet_getcstring(argv, 0);
    regex                 = new_abstract_pcre2_regex(re_string, argv, 3, argc);
  }
  else
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }
  const char* input   = janet_getcstring(argv, 1);
  const char* replace = janet_getcstring(argv, 2);
  return pcre2_replace_w_options(regex, input, replace, true);
}

/****************/
/* Module Entry */
/****************/

JANET_MODULE_ENTRY(JanetTable* env)
{
  JanetRegExt cfuns[] = { JANET_REG("std-compile", cfun_std_compile),
                          JANET_REG("std-contains", cfun_std_contains),
                          JANET_REG("std-find", cfun_std_find),
                          JANET_REG("std-find-all", cfun_std_findall),
                          JANET_REG("std-replace", cfun_std_replace),
                          JANET_REG("std-replace-all", cfun_std_replace_all),
                          JANET_REG("pcre2-compile", cfun_pcre2_compile),
                          JANET_REG("pcre2-contains", cfun_pcre2_contains),
                          JANET_REG("pcre2-match", cfun_pcre2_match),
                          JANET_REG("pcre2-find", cfun_pcre2_find),
                          JANET_REG("pcre2-find-all", cfun_pcre2_findall),
                          JANET_REG("pcre2-replace", cfun_pcre2_replace),
                          JANET_REG("pcre2-replace-all", cfun_pcre2_replace_all),
                          JANET_REG_END };
  janet_cfuns_ext(env, "re-janet", cfuns);
}

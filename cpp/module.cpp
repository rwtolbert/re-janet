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

#include "wrap_pcre2.h"
#include "wrap_std_regex.h"
#include "results.h"

#include "module.h"

#include <iostream>
#include <sstream>

/*****************/
/* C++ Functions */
/*****************/

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

JANET_FN(cfun_std_match, "(jre/_std-match regex text &opt start-index)",
         R"(Match a pre-compiled regex or regex string to an input string.

Return array of captured values.
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
  else if (janet_checkabstract(argv[0], &regex_type))
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :std");
  }

  int startIndex = 0;
  if (argc == 3)
  {
    startIndex = janet_getinteger(argv, 2);
    if (startIndex <= 0)
      startIndex = 0;
  }

  const char* input  = janet_getcstring(argv, 1);
  auto        result = false;
  if (input && regex->re)
  {
    std::string s(input);
    auto        searchBegin = std::sregex_iterator(s.begin(), s.end(), *regex->re);
    if (localRegex)
      set_gcmark(regex, 0);
    return janet_wrap_array(extract_array_from_iterator(s, searchBegin));
  }
  if (localRegex)
    set_gcmark(regex, 0);
  return janet_wrap_nil();
}

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
  else if (janet_checkabstract(argv[0], &regex_type))
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :std");
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
  else if (janet_checkabstract(argv[0], &regex_type))
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :std");
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
  else if (janet_checkabstract(argv[0], &regex_type))
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :std");
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
  else if (janet_checkabstract(argv[0], &regex_type))
  {
    regex = (JanetRegex*)janet_getabstract(argv, 0, &regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :std");
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

  const char* input      = janet_getcstring(argv, 1);
  PCRE2_SIZE  startIndex = 0;
  return janet_wrap_boolean(pcre2_contains(regex, input));
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
  else if (janet_checkabstract(argv[0], &pcre2_regex_type))
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :pcre2");
  }

  PCRE2_SIZE startIndex = 0;
  if (argc == 3)
  {
    startIndex = janet_getinteger(argv, 2);
    if (startIndex <= 0)
      startIndex = 0;
  }

  const char* input = janet_getcstring(argv, 1);

  bool firstOnly = true;
  auto matches   = pcre2_match(regex, input, startIndex, options, firstOnly);
  if (matches.empty())
    return janet_wrap_nil();

  return janet_wrap_integer(matches.at(0).begin);
}

JANET_FN(cfun_pcre2_findall, "(jre/_pcre2-findall regex text &opt start-index)",
         R"(Find position of all matches of regex in text.)")
{
  janet_arity(argc, 2, 3);

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
  else if (janet_checkabstract(argv[0], &pcre2_regex_type))
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :pcre2");
  }

  PCRE2_SIZE startIndex = 0;
  if (argc == 3)
  {
    startIndex = janet_getinteger(argv, 2);
    if (startIndex <= 0)
      startIndex = 0;
  }

  const char* input = janet_getcstring(argv, 1);

  auto matches = pcre2_match(regex, input, startIndex, options);

  JanetArray* array = janet_array(matches.size());
  for (auto&& m : matches)
  {
    janet_array_push(array, janet_wrap_number(m.begin));
  }
  return janet_wrap_array(array);
}

JANET_FN(cfun_pcre2_match, "(jre/_pcre2-match regex text &opt start-index)", R"(Return array of captured values.)")
{
  janet_arity(argc, 2, 3);

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
  else if (janet_checkabstract(argv[0], &pcre2_regex_type))
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :pcre2");
  }

  PCRE2_SIZE startIndex = 0;
  if (argc == 3)
  {
    startIndex = janet_getinteger(argv, 2);
    if (startIndex <= 0)
      startIndex = 0;
  }

  const char* input = janet_getcstring(argv, 1);

  auto matches = pcre2_match(regex, input, startIndex, options);
  auto array   = MatchResultsToArray(matches);

  if (localRegex)
    pcre2_set_gcmark(regex, 0);

  return array;
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
      printf("ran out of memory, input: %zu needed: %zu\n", strlen(input), outlen);
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
  else if (janet_checkabstract(argv[0], &pcre2_regex_type))
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :pcre2");
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
  else if (janet_checkabstract(argv[0], &pcre2_regex_type))
  {
    regex = (JanetPCRE2Regex*)janet_getabstract(argv, 0, &pcre2_regex_type);
  }
  else
  {
    janet_panic("First argument must be a string or regex compiled with :pcre2");
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
                          JANET_REG("std-match", cfun_std_match),
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

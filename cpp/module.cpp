#include <janet.h>

#include <iostream>
#include <sstream>

#include "module.h"

/*****************/
/* C++ Functions */
/*****************/

namespace {

struct JanetRegex {
  JanetGCObject gc;
  std::regex *re = nullptr;
  std::vector<std::string> flags;
  std::string pattern = "";
};

int set_gc(void *data, size_t len) {
  (void)len;
  if (data) {
    JanetRegex *re = (JanetRegex *)data;
    if (re->re) {
      delete (re->re);
      re->re = nullptr;
    }
    re->flags.clear();
    re->pattern = "";
  }
  return 0;
}

int set_gcmark(void *data, size_t len) {
  (void)len;
  return 0;
}

void set_tostring(void *data, JanetBuffer *buffer) {
  JanetRegex *re = (JanetRegex *)data;
  janet_buffer_push_cstring(buffer, "pattern: '");
  janet_buffer_push_cstring(buffer, re->pattern.c_str());
  janet_buffer_push_cstring(buffer, "'");
  if (!re->flags.empty()) {
    janet_buffer_push_cstring(buffer, " flags: (");
    bool first = true;
    for (int32_t i = 0; i < re->flags.size(); ++i) {
      if (!first)
        janet_buffer_push_cstring(buffer, " ");
      janet_buffer_push_cstring(buffer, ":");
      janet_buffer_push_cstring(buffer, re->flags[i].c_str());
      first = false;
    }
    janet_buffer_push_cstring(buffer, ")");
  } else {
    janet_buffer_push_cstring(buffer, " flags: ()");
  }
}

const JanetKeyword ecmascript = janet_ckeyword("ecmascript");
const JanetKeyword ignorecase = janet_ckeyword("ignorecase");
const JanetKeyword optimize = janet_ckeyword("optimize");
const JanetKeyword collate = janet_ckeyword("collate");
const JanetKeyword basic = janet_ckeyword("basic");
const JanetKeyword extended = janet_ckeyword("extended");
const JanetKeyword awk = janet_ckeyword("awk");
const JanetKeyword grep = janet_ckeyword("grep");
const JanetKeyword egrep = janet_ckeyword("egrep");

const JanetAbstractType regex_type = {
    .name = "jre",
    .gc = set_gc,
    .gcmark = set_gcmark,
    .get = NULL,
    .put = NULL,
    .marshal = NULL,
    .unmarshal = NULL,
    .tostring = set_tostring,
    .compare = NULL,
    .hash = NULL,
    .next = NULL,
    .call = NULL,
    .length = NULL,
    .bytes = NULL,
};

std::string get_allow_flags() {
  std::ostringstream os;
  os << ":ignorecase ";
  os << ":optimize ";
  os << ":collate ";
  os << ":ecmascript ";
  os << ":basic ";
  os << ":extended ";
  os << ":awk ";
  os << ":grep ";
  os << ":egrep ";
  return os.str();
}

std::regex::flag_type get_flag_type(JanetKeyword kw) {
  if (kw == ignorecase) {
    return std::regex::icase;
  } else if (kw == optimize) {
    return std::regex::optimize;
  } else if (kw == collate) {
    return std::regex::collate;
  } else if (kw == basic) {
    return std::regex::basic;
  } else if (kw == extended) {
    return std::regex::extended;
  } else if (kw == awk) {
    return std::regex::awk;
  } else if (kw == grep) {
    return std::regex::grep;
  } else if (kw == egrep) {
    return std::regex::egrep;
  }
  janet_panicf(":%s is not a valid regex flag.\n  Flags should be from list %s",
               kw, get_allow_flags().c_str());
  return std::regex::ECMAScript;
}

JanetRegex *new_abstract_regex(const char *input, const Janet *argv,
                               int32_t flag_start, int32_t argc) {
  JanetRegex *regex =
      (JanetRegex *)janet_abstract(&regex_type, sizeof(JanetRegex));

  std::regex::flag_type flags = std::regex::ECMAScript;

  for (int32_t i = flag_start; i < argc; ++i) {
    if (!janet_checktype(argv[i], JANET_KEYWORD))
      janet_panicf("Regex flags must be keyword from:  %s",
                   get_allow_flags().c_str(), argv[i]);

    auto arg = janet_getkeyword(argv, i);
    if (arg) {
      flags |= get_flag_type(arg);
      JanetBuffer temp;
      janet_buffer_init(&temp, 0);
      janet_buffer_push_string(&temp, arg);
      auto str = std::string((const char *)temp.data, temp.count);
      regex->flags.push_back(str);
      janet_buffer_deinit(&temp);
    }
  }

  if (input) {
    try {
      if (flags) {
        regex->re = new std::regex(input, flags);
      } else {
        regex->re = new std::regex(input);
      }
      regex->pattern = std::string(input);
    } catch (const std::regex_error &e) {
      janet_panicf("%s", e.what());
    }
  }

  return regex;
}

JanetTable *extract_table_from_match(const std::string &input,
                                     const std::smatch &match) {
  JanetTable *results = janet_table(5);

  janet_table_put(results, janet_ckeywordv("begin"),
                  janet_wrap_integer(match.position()));
  janet_table_put(results, janet_ckeywordv("end"),
                  janet_wrap_integer(match.position() + match.length()));
  janet_table_put(results, janet_ckeywordv("prefix"),
                  janet_wrap_string(
                      janet_string((const uint8_t *)match.prefix().str().data(),
                                   match.prefix().str().size())));
  janet_table_put(results, janet_ckeywordv("suffix"),
                  janet_wrap_string(
                      janet_string((const uint8_t *)match.suffix().str().data(),
                                   match.suffix().str().size())));

  Janet matches[match.size()];
  for (size_t j = 0; j < match.size(); ++j) {
    JanetTable *group = janet_table(4);
    auto &&sub = match[j];
    auto bgn = std::distance(input.begin(), sub.first);
    auto end = bgn + sub.length();

    janet_table_put(group, janet_ckeywordv("index"), janet_wrap_integer(j));
    janet_table_put(group, janet_ckeywordv("str"),
                    janet_wrap_string(janet_string(
                        (const uint8_t *)sub.str().data(), sub.str().size())));
    janet_table_put(group, janet_ckeywordv("matched"),
                    janet_wrap_boolean(sub.matched));
    janet_table_put(group, janet_ckeywordv("begin"), janet_wrap_integer(bgn));
    janet_table_put(group, janet_ckeywordv("end"), janet_wrap_integer(end));
    matches[j] = janet_wrap_table(group);
  }
  janet_table_put(results, janet_ckeywordv("groups"),
                  janet_wrap_tuple(janet_tuple_n(matches, match.size())));
  return results;
}

} // namespace

JANET_FN(cfun_re_compile, "(re-janet/compile)",
         "Compile regex for repeated use.") {
  janet_arity(argc, 1, 6);
  const char *input = janet_getcstring(argv, 0);
  JanetRegex *regex = new_abstract_regex(input, argv, 1, argc);
  return janet_wrap_abstract(regex);
}

JANET_FN(cfun_re_match, "(re-janet/match)",
         "Match a pre-compiled regex to an input string") {
  janet_fixarity(argc, 2);

  JanetRegex *regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING)) {
    const char *re_string = janet_getcstring(argv, 0);
    regex = new_abstract_regex(re_string, argv, 0, 0);
  } else {
    regex = (JanetRegex *)janet_getabstract(argv, 0, &regex_type);
  }

  const char *input = janet_getcstring(argv, 1);
  auto result = false;
  if (input && regex->re) {
    std::string s(input);
    std::smatch m;
    result = std::regex_match(s, m, *regex->re);
    if (result) {
      JanetTable *table = extract_table_from_match(s, m);
      return janet_wrap_table(table);
    }
  }
  return janet_wrap_nil();
}

JANET_FN(cfun_re_search, "(re-janet/search)",
         "Search a pre-compiled regex in an input string") {
  janet_fixarity(argc, 2);

  JanetRegex *regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING)) {
    const char *re_string = janet_getcstring(argv, 0);
    regex = new_abstract_regex(re_string, argv, 0, 0);
  } else {
    regex = (JanetRegex *)janet_getabstract(argv, 0, &regex_type);
  }

  const char *input = janet_getcstring(argv, 1);
  auto result = false;
  if (input && regex->re) {
    std::string s(input);

    auto search_begin = std::sregex_iterator(s.begin(), s.end(), *regex->re);
    auto search_end = std::sregex_iterator();
    auto count = std::distance(search_begin, search_end);

    if (count > 0) {
      JanetArray *ja = janet_array(count);
      for (std::sregex_iterator iter = search_begin; iter != search_end;
           ++iter) {
        JanetTable *results = extract_table_from_match(s, *iter);
        janet_array_push(ja, janet_wrap_table(results));
      }
      return janet_wrap_array(ja);
    }
  }
  return janet_wrap_nil();
}

JANET_FN(cfun_re_replace, "jre/replace",
         "Search for regex and replace with the provided string.") {
  janet_fixarity(argc, 3);
  JanetRegex *regex = NULL;
  if (janet_checktype(argv[0], JANET_STRING)) {
    const char *re_string = janet_getcstring(argv, 0);
    regex = new_abstract_regex(re_string, argv, 3, argc);
  } else {
    regex = (JanetRegex *)janet_getabstract(argv, 0, &regex_type);
  }
  const char *input = janet_getcstring(argv, 1);
  const char *replace = janet_getcstring(argv, 2);
  if (input && replace && regex->re) {
    auto result = std::regex_replace(input, *regex->re, replace);
    return janet_wrap_string(janet_cstring(result.c_str()));
  }
  return janet_wrap_string(janet_cstring(input));
}

/****************/
/* Module Entry */
/****************/

JANET_MODULE_ENTRY(JanetTable *env) {
  JanetRegExt cfuns[] = {JANET_REG("compile", cfun_re_compile),
                         JANET_REG("match", cfun_re_match),
                         JANET_REG("search", cfun_re_search),
                         JANET_REG("replace", cfun_re_replace), JANET_REG_END};

  janet_def_sm(env, ":ignorecase", janet_wrap_keyword(ignorecase),
               "Character matching should be performed without regard to case.",
               NULL, 0);
  janet_def_sm(env, ":optimize", janet_wrap_keyword(optimize),
               "Instructs the regular expression engine to make matching "
               "faster, at the expense of slower construction.",
               NULL, 0);
  janet_def_sm(env, ":collate", janet_wrap_keyword(collate),
               "Character ranges of the form '[a-b]' will be locale sensitive.",
               NULL, 0);

  janet_def_sm(env, ":ecmascript", janet_wrap_keyword(ecmascript),
               "Default match type", NULL, 0);
  janet_def_sm(env, ":basic", janet_wrap_keyword(basic),
               "Use the basic POSIX regular expression grammar.", NULL, 0);
  janet_def_sm(env, ":extended", janet_wrap_keyword(extended),
               "Use the extended POSIX regular expression grammar.", NULL, 0);
  janet_def_sm(
      env, ":awk", janet_wrap_keyword(awk),
      "Use the regular expression grammar used by the awk utility in POSIX.",
      NULL, 0);
  janet_def_sm(env, ":grep", janet_wrap_keyword(grep),
               "Use the regular expression grammar used by the grep utility.",
               NULL, 0);
  janet_def_sm(env, ":egrep", janet_wrap_keyword(egrep),
               "Use the regular expression grammar used by the egrep utility.",
               NULL, 0);

  janet_cfuns_ext(env, "re-janet", cfuns);
}
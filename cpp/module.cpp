#include <janet.h>

#include <iostream>
#include <sstream>

#include "module.h"

/*****************/
/* C++ Functions */
/*****************/

struct JanetRegex {
  JanetGCObject gc;
  std::regex *re = nullptr;
  std::vector<std::string> flags;
  std::string pattern = "";
};

static int set_gc(void *data, size_t len) {
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

static int set_gcmark(void *data, size_t len) {
  (void)len;
  return 0;
}

static void set_tostring(void *data, JanetBuffer *buffer) {
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

static const JanetKeyword ecmascript = janet_ckeyword("ecmascript");
static const JanetKeyword ignorecase = janet_ckeyword("ignorecase");
static const JanetKeyword optimize = janet_ckeyword("optimize");
static const JanetKeyword collate = janet_ckeyword("collate");
static const JanetKeyword basic = janet_ckeyword("basic");
static const JanetKeyword extended = janet_ckeyword("extended");
static const JanetKeyword awk = janet_ckeyword("awk");
static const JanetKeyword grep = janet_ckeyword("grep");
static const JanetKeyword egrep = janet_ckeyword("egrep");
static const JanetKeyword multiline = janet_ckeyword("multiline");

static const JanetAbstractType regex_type = {
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

JANET_FN(cfun_hello_native, "(re-janet/hello-native)",
         "Evaluate to \"Hello!\". but implemented in C.") {
  janet_fixarity(argc, 0);
  (void)argv;
  return janet_cstringv("Hello!");
}

static JanetRegex *new_abstract_regex() {
  JanetRegex *regex =
      (JanetRegex *)janet_abstract(&regex_type, sizeof(JanetRegex));
  return regex;
}

static std::string get_allow_flags() {
  std::ostringstream os;
  os << ":ignorecase ";
  os << ":optimize ";
  os << ":collate ";
  // #if __cplusplus > 201402L // C++17 or greater
  os << ":multiline";
  // #endif
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
  // #if __cplusplus > 201402L // C++17 or greater
  else if (kw == multiline) {
    return std::regex::multiline;
  }
  // #endif
  janet_panicf(":%s is not a valid regex flag.\n  Flags should be from list %s",
               kw, get_allow_flags().c_str());
  return std::regex::ECMAScript;
}

JANET_FN(cfun_re_compile, "(re-janet/compile)",
         "Compile regex for repeated use.") {
  janet_arity(argc, 1, 6);
  const char *input = janet_getcstring(argv, 0);

  JanetRegex *regex = new_abstract_regex();

  std::regex::flag_type flags = std::regex::ECMAScript;
  for (int32_t i = 1; i < argc; ++i) {
    if (!janet_checktype(argv[i], JANET_KEYWORD))
      janet_panicf(
          "Regex flags must be keyword from: :icase, :optimize, :collate, "
          ":ecmascript, :basic, :extended, :awk, :grep, :egrep",
          argv[i]);

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
  return janet_wrap_abstract(regex);
}

JANET_FN(cfun_re_match, "(re-janet/match)",
         "Match a pre-compiled regex to an input string") {
  janet_fixarity(argc, 2);

  JanetRegex *regex = (JanetRegex *)janet_getabstract(argv, 0, &regex_type);
  const char *input = janet_getcstring(argv, 1);
  auto result = false;
  if (input && regex->re) {
    std::cmatch m;
    result = std::regex_match(input, m, *regex->re);
  }
  return janet_wrap_boolean(result);
}

/****************/
/* Module Entry */
/****************/

JANET_MODULE_ENTRY(JanetTable *env) {
  JanetRegExt cfuns[] = {JANET_REG("hello-native", cfun_hello_native),
                         JANET_REG("compile", cfun_re_compile),
                         JANET_REG("match", cfun_re_match), JANET_REG_END};

  janet_def_sm(env, ":ignorecase", janet_wrap_keyword(ignorecase),
               "Character matching should be performed without regard to case.",
               NULL, 0);
  janet_def_sm(env, ":optimize", janet_wrap_keyword(optimize),
               "Instructs the regular expression engine to make matching "
               "faster, at the expense of slower construction.",
               NULL, 0);
  janet_def_sm(
      env, ":multiline", janet_wrap_keyword(multiline),
      "Specifies that ^ shall match the beginning of a line and $ shall match "
      "the end of a line, if :ecmascript is selected. ",
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

  janet_cfuns_ext(env, "re-janet", cfuns);
}
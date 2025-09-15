#include "wrap_pcre2.h"
#include "results.h"

#include <iostream>
#include <sstream>

namespace
{
const char* pcre2_allowed = "[:ignorecase]";
const char* ignorecase    = "ignorecase";

uint32_t
get_pcre2_flag_type(JanetKeyword kw)
{
  if (kw == janet_ckeyword(ignorecase))
  {
    return PCRE2_CASELESS;
  }
  return 0;
}
}

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

const JanetAbstractType pcre2_regex_type = {
  .name     = "pcre2",
  .gc       = pcre2_set_gc,
  .gcmark   = pcre2_set_gcmark,
  .tostring = pcre2_set_tostring,
};

JanetPCRE2Regex*
new_abstract_pcre2_regex(const char* input, const Janet* argv, int32_t flag_start, int32_t argc)
{
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

bool
pcre2_contains(const JanetPCRE2Regex* regex, const char* subject, PCRE2_SIZE startIndex)
{
  auto options        = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH;
  auto match_data     = pcre2_match_data_create_from_pattern(regex->re, NULL);
  auto subject_length = strlen(subject);
  int  rc;
  if (regex->jit)
  {
    rc = pcre2_jit_match(regex->re,           /* the compiled pattern */
                         (PCRE2_SPTR)subject, /* the subject string */
                         subject_length,      /* the length of the subject */
                         startIndex,          /* start at offset in the subject */
                         options,             /* default options */
                         match_data,          /* block for storing the result */
                         NULL);
  }
  else
  {
    std::cerr << "non jit fallback" << std::endl;
    rc = pcre2_match(regex->re,           /* the compiled pattern */
                     (PCRE2_SPTR)subject, /* the subject string */
                     subject_length,      /* the length of the subject */
                     startIndex,          /* start at offset in the subject */
                     options,             /* default options */
                     match_data,          /* block for storing the result */
                     NULL);
  }

  pcre2_match_data_free(match_data);
  return rc > 0;
}

std::vector<ReMatch>
pcre2_match(const JanetPCRE2Regex* regex, const char* subject, PCRE2_SIZE startIndex, PCRE2_SIZE options,
            bool firstOnly)
{
  std::vector<ReMatch> matches;

  auto match_data     = pcre2_match_data_create_from_pattern(regex->re, NULL);
  auto subject_length = strlen(subject);
  int  rc;
  if (regex->jit)
  {
    rc = pcre2_jit_match(regex->re,           /* the compiled pattern */
                         (PCRE2_SPTR)subject, /* the subject string */
                         subject_length,      /* the length of the subject */
                         startIndex,          /* start at offset in the subject */
                         options,             /* default options */
                         match_data,          /* block for storing the result */
                         NULL);
  }
  else
  {
    rc = pcre2_match(regex->re,           /* the compiled pattern */
                     (PCRE2_SPTR)subject, /* the subject string */
                     subject_length,      /* the length of the subject */
                     startIndex,          /* start at offset in the subject */
                     options,             /* default options */
                     match_data,          /* block for storing the result */
                     NULL);
  }

  // handle the case of failed match
  // TODO - propagate error in janet_panic
  if (rc <= 0)
  {
    pcre2_match_data_free(match_data);
    return matches;
  }

  auto ovector = pcre2_get_ovector_pointer(match_data);

  // first match is entire match, rest are capture groups
  PCRE2_SPTR substring_start  = (PCRE2_SPTR)subject + ovector[0];
  PCRE2_SIZE substring_length = ovector[1] - ovector[0];

  ReMatch firstMatch;
  firstMatch.begin = ovector[0];
  firstMatch.end   = ovector[1];
  firstMatch.val   = std::string((const char*)substring_start, substring_length);

  if (rc > 1)
  {
    for (int i = 1; i < rc; i++)
    {
      PCRE2_SPTR substring_start  = (PCRE2_SPTR)subject + ovector[2 * i];
      PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
      if (substring_length > 0)
      {
        ReMatch group;
        group.index = i;
        group.begin = ovector[2 * i];
        group.end   = ovector[2 * i + 1];
        group.val   = std::string((const char*)substring_start, substring_length);
        firstMatch.groups.emplace_back(group);
      }
    }
  }
  matches.emplace_back(firstMatch);

  if (firstOnly)
  {
    pcre2_match_data_free(match_data);
    return matches;
  }

  /* Before running the loop, check for UTF-8 and whether CRLF is a valid newline
     sequence. First, find the options with which the regex was compiled and extract
     the UTF state. */

  uint32_t option_bits;
  (void)pcre2_pattern_info(regex->re, PCRE2_INFO_ALLOPTIONS, &option_bits);
  int utf8 = (option_bits & PCRE2_UTF) != 0;

  /* Now find the newline convention and see whether CRLF is a valid newline
  sequence. */

  uint32_t newline;
  (void)pcre2_pattern_info(regex->re, PCRE2_INFO_NEWLINE, &newline);
  int crlf_is_newline
      = newline == PCRE2_NEWLINE_ANY || newline == PCRE2_NEWLINE_CRLF || newline == PCRE2_NEWLINE_ANYCRLF;

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

        start_offset = startchar + 1; /* Advance by one character. */
        if (utf8)                     /* If UTF-8, it may be more  */
        {                             /*   than one code unit.     */
          for (; start_offset < subject_length; start_offset++)
            if ((subject[start_offset] & 0xc0) != 0x80)
              break;
        }
      }
    }

    /* Run the next matching operation */
    if (regex->jit)
    {
      rc = pcre2_jit_match(regex->re,           /* the compiled pattern */
                           (PCRE2_SPTR)subject, /* the subject string */
                           subject_length,      /* the length of the subject */
                           start_offset,        /* start at offset in the subject */
                           options,             /* default options */
                           match_data,          /* block for storing the result */
                           NULL);
    }
    else
    {
      rc = pcre2_match(regex->re,           /* the compiled pattern */
                       (PCRE2_SPTR)subject, /* the subject string */
                       subject_length,      /* the length of the subject */
                       start_offset,        /* start at offset in the subject */
                       options,             /* default options */
                       match_data,          /* block for storing the result */
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
        break;                                 /* All matches found */
      ovector[1] = start_offset + 1;           /* Advance one code unit */
      if (crlf_is_newline &&                   /* If CRLF is a newline & */
          start_offset < subject_length - 1 && /* we are at CRLF, */
          subject[start_offset] == '\r' && subject[start_offset + 1] == '\n')
        ovector[1] += 1;                    /* Advance by one more. */
      else if (utf8)                        /* Otherwise, ensure we */
      {                                     /* advance a whole UTF-8 */
        while (ovector[1] < subject_length) /* character. */
        {
          if ((subject[ovector[1]] & 0xc0) != 0x80)
            break;
          ovector[1] += 1;
        }
      }
      continue; /* Go round the loop again */
    }

    /* Other matching errors are not recoverable. */
    // TODO - propagate error in janet_panic
    if (rc < 0)
    {
      pcre2_match_data_free(match_data);
      // zero out any temp results
      matches.clear();
      return matches;
    }

    // first match is entire match, rest are capture groups
    // JanetTable* matchResults     = janet_table(3);
    PCRE2_SPTR substring_start  = (PCRE2_SPTR)subject + ovector[0];
    PCRE2_SIZE substring_length = ovector[1] - ovector[0];

    ReMatch nextMatch;
    nextMatch.begin = ovector[0];
    nextMatch.end   = ovector[1];
    nextMatch.val   = std::string((const char*)substring_start, substring_length);

    if (rc > 1)
    {
      for (int i = 1; i < rc; i++)
      {
        PCRE2_SPTR substring_start  = (PCRE2_SPTR)subject + ovector[2 * i];
        PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
        if (substring_length > 0)
        {
          ReMatch group;
          group.index = i;
          group.begin = ovector[2 * i];
          group.end   = ovector[2 * i + 1];
          group.val   = std::string((const char*)substring_start, substring_length);
          nextMatch.groups.emplace_back(group);
        }
      }
    }
    matches.emplace_back(nextMatch);
  }

  pcre2_match_data_free(match_data);
  return matches;
}

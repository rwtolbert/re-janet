#pragma once

#include <janet.h>

#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <string>
#include <vector>

#include "results.h"

struct JanetPCRE2Regex
{
  JanetGCObject             gc;
  pcre2_code*               re      = nullptr;
  std::string*              pattern = nullptr;
  std::vector<std::string>* flags   = nullptr;
  bool                      jit     = false;
};

extern JanetAbstractType pcre2_regex_type;

JanetPCRE2Regex* new_abstract_pcre2_regex(const char* input, const Janet* argv, int32_t flag_start, int32_t argc);

int  pcre2_set_gc(void* data, size_t len);
int  pcre2_set_gcmark(void* data, size_t len);
void pcre2_set_tostring(void* data, JanetBuffer* buffer);

std::vector<ReMatch> pcre2_match(const JanetPCRE2Regex* regex, const char* subject, PCRE2_SIZE startIndex,
                                 PCRE2_SIZE options, bool firstOnly = false);
bool                 pcre2_contains(const JanetPCRE2Regex* regex, const char* subject, PCRE2_SIZE startIndex = 0);

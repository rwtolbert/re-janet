#pragma once

#include <janet.h>

#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <string>
#include <vector>

struct JanetPCRE2Regex
{
  JanetGCObject             gc;
  pcre2_code*               re      = nullptr;
  std::string*              pattern = nullptr;
  std::vector<std::string>* flags   = nullptr;
  bool                      jit     = false;
};

extern const JanetAbstractType pcre2_regex_type;

JanetPCRE2Regex* new_abstract_pcre2_regex(const char* input, const Janet* argv, int32_t flag_start, int32_t argc);

void initialize_pcre2_regex_type();

int  pcre2_set_gc(void* data, size_t len);
int  pcre2_set_gcmark(void* data, size_t len);
void pcre2_set_tostring(void* data, JanetBuffer* buffer);

Janet pcre2_match(const JanetPCRE2Regex* regex, const char* subject, PCRE2_SIZE startIndex, PCRE2_SIZE options,
                  bool firstOnly = false);

#pragma once

#include <janet.h>
#include <string>
#include <vector>
#include <regex>

struct JanetRegex
{
  JanetGCObject             gc;
  std::regex*               re      = nullptr;
  std::string*              pattern = nullptr;
  std::vector<std::string>* flags   = nullptr;
};

extern JanetAbstractType regex_type;

int set_gc(void* data, size_t len);
int set_gcmark(void* data, size_t len);

JanetRegex* new_abstract_regex(const char* input, const Janet* argv, int32_t flag_start, int32_t argc);

JanetTable* extract_table_from_match(const std::string& input, const std::smatch& match);

JanetArray* extract_array_from_iterator(const std::string& input, std::sregex_iterator& iter);

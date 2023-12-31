(declare-project
  :name "re-janet"
  :description ```Janet wrapper around C++ std::regex ```
  :version "0.2.1"
  :dependencies ["https://github.com/janet-lang/spork.git"])

(declare-native
  :name "jre"
  :source @["cpp/module.cpp"])

(declare-project
  :name "re-janet"
  :description ```Janet wrapper around C++ std::regex ```
  :version "0.3.0"
  :dependencies ["https://github.com/janet-lang/spork.git"])

(declare-native
  :name "jre"
  :source @["cpp/module.cpp"])

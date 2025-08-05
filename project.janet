(declare-project
  :name "re-janet"
  :description ```Janet wrapper around C++ std::regex ```
  :version "0.3.1"
  :dependencies ["https://github.com/janet-lang/spork.git"])

(declare-native
  :name "jre"
  :source @["cpp/module.cpp"]
  :c++flags @["-I/opt/homebrew/include"]
  :libs @["/opt/homebrew/lib/libpcre2-8.a"])

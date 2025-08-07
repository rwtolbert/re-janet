(declare-project
  :name "re-janet"
  :description ```Janet wrapper around C++ std::regex ```
  :version "0.3.1"
  :dependencies ["https://github.com/janet-lang/spork.git"])

(def cflags @["-I_build/pcre2"])

(declare-native
  :name "jre"
  :source @["cpp/module.cpp"]
  :use-rpath true
  :c++flags cflags
  :libs (dyn *lflags*))

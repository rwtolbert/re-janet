(declare-project
  :name "re-janet"
  :description ```Janet wrapper around C++ std::regex ```
  :version "0.3.1"
  :dependencies ["https://github.com/janet-lang/spork.git"])

(setdyn :verbose true)

(def cflags @["-I_build/pcre2"])

(pp (dyn *lflags*))

(declare-native
  :name "jre"
  :source @["cpp/module.cpp"]
  :use-rpath true
  :c++flags cflags
  :libs (dyn *lflags*))

(declare-project
  :name "re-janet"
  :description ```Janet wrapper around C++ std::regex ```
  :version "0.3.1"
  :dependencies ["https://github.com/janet-lang/spork.git"])

(def flags (cond
             (= (os/which) :linux) @[]
             (= (os/which) :macos) @["-I/opt/homebrew/include"]))


(def libs (cond
            (= (os/which) :linux) @["-lpcre2-8"]
            (= (os/which) :macos) @["/opt/homebrew/lib/libpcre2-8.a"]))

(declare-native
  :name "jre"
  :source @["cpp/module.cpp"]
  :c++flags flags
  :libs libs)

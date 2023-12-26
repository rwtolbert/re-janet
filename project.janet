(declare-project
  :name "re-janet"
  :description ```Janet wrapper around C++ std::regex ```
  :version "0.1.0"
  :dependencies ["https://github.com/janet-lang/spork.git"])

# (declare-source
#   :prefix "jre"
#   :source ["jre/init.janet"])

(declare-native
  :name "jre"
  :source @["cpp/module.cpp"])

(use spork/declare-cc)

# pull info.jdn so we don't duplicate symbols there and
# in declare-project below
(def info (-> (slurp "./bundle/info.jdn") parse))

(declare-project
  :name (info :name)
  :description (info :description)
  :version (info :version)
  :dependencies (info :dependencies))

# force default to release unless specifically requested
(when (not (os/getenv "JANET_BUILD_TYPE"))
  (setdyn :build-type :release))

###########
(import janet-native-tools :as jnt)

(import spork/sh)
(import spork/path)

# make sure we can find cmake and make
(jnt/require-git)
(jnt/require-cmake)
(jnt/require-make)

(def- build-type (get (curenv) :build-type))
(def- build-dir (path/join "_build" build-type))

#####################################################
# PCRE2 build
(def- PCRE2-tag "pcre2-10.45")
(defn- update-submodules []
  (jnt/git "submodule" "update" "--init" "--recursive")
  (os/cd "libs/pcre2")
  (jnt/git "checkout" PCRE2-tag)
  (os/cd "../..")
  (jnt/git "submodule" "update" "--init" "--recursive"))

(def- pcre2-build-dir "_build/pcre2")
(def- cmake-flags @["-DPCRE2_SUPPORT_JIT=ON"
                    "-DPCRE2_STATIC_PIC=ON"
                    "-DBUILD_SHARED_LIBS=OFF"])

(def [build-pcre2 clean-pcre2]
  (jnt/declare-cmake :name "pcre2"
                     :source-dir "libs/pcre2"
                     :build-dir pcre2-build-dir
                     :build-type "Release"
                     :cmake-flags cmake-flags))

(def pcre2-static-lib
  (if (= (os/which) :windows)
    (jnt/gen-static-libname "pcre2-8-static")
    (jnt/gen-static-libname "pcre2-8")))

(defn- clean-static-lib
  "Remove old static lib from jre directory"
  []
  (print "removing static lib")
  (let [a (path/join "./jre" pcre2-static-lib)]
    (when (sh/exists? a)
      (sh/rm a))))

(defn- copy-static-lib
  "Copy static lib to jre directory for install"
  []
  (print "copying static lib")
  (let [infile (string/format "%s/%s" pcre2-build-dir pcre2-static-lib)
        outfile (string/format "jre/%s" pcre2-static-lib)]
    (when (sh/exists? infile)
      (sh/copy-file infile outfile))))

# create new task to build C PCRE2 static lib
(task "build-pcre2" []
      (update-submodules)
      (clean-static-lib)
      (build-pcre2)
      (copy-static-lib))

# attach this task to run during pre-build
(task "pre-build" ["build-pcre2"])

# task/hook for cleaning pcre2
# `janet-pm clean-all` will remove the `_build` directory which
# also removes the pcre2 build dir
(task "clean-pcre2" [] (clean-pcre2))

#########################################################

(defn- gen-lflags []
  (if (= (os/which) :windows)
    @[(string/format "/LIBPATH:./%s" pcre2-build-dir) "pcre2-8-static.lib"]
    @[(string/format "-L%s" pcre2-build-dir) "-lpcre2-8"]))

(def- cflags @[(string/format "-I%s" pcre2-build-dir)])

(declare-source
  :source ["jre"])

(declare-native
  :name "jre/native"
  :source @["cpp/module.cpp"
            "cpp/wrap_pcre2.cpp"
            "cpp/wrap_std_regex.cpp"
            "cpp/results.cpp"]
  :use-rpath true
  :c++flags cflags
  :lflags (gen-lflags))

# create a new task to run the ldflags fixup
(task "fix-up-ldflags" [] (jnt/fix-up-ldflags))

# attach this task to the post-install hook
(task "post-install" ["fix-up-ldflags"])

#(task "list-installed" [] (jnt/list-installed))


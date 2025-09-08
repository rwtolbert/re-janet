(use spork/declare-cc)

(declare-project
  :name "re-janet"
  :description ```Janet wrapper around PCRE2 and C++ std::regex ```
  :version "0.4.1"
  :dependencies [])

(def- PCRE2-tag "pcre2-10.45")

# force default to release unless specifically requested
(when (not (os/getenv "JANET_BUILD_TYPE"))
  (setdyn :build-type :release))

###########
(import spork/pm)
(import spork/sh)
(import spork/path)

(setdyn :verbose true)

(def- build-type (get (curenv) :build-type))
(def- build-dir (path/join "_build" build-type))

(defdyn *cmakepath* "What cmake command to use")
(defdyn *makepath* "What make command to use")
(defdyn *ninjapath* "What ninja command to use")

(defn error-exit [msg &opt code]
  (default code 1)
  (printf msg)
  (os/exit code))

(defn set-command [cmd todyn]
  "Look for executable on PATH"
  (if (sh/which cmd)
    (setdyn todyn (sh/which cmd))
    (error-exit (string/format "Unable to find command: %s" cmd))))

(set-command "cmake" *cmakepath*)
(set-command "ninja" *ninjapath*)

(defn- cmake
  "Make a call to cmake."
  [& args]
  (printf "cmake %j" args)
  (sh/exec (dyn *cmakepath* "cmake") ;args))

(defn- update-submodules []
  (pm/git "submodule" "update" "--init" "--recursive")
  (os/cd "libs/pcre2")
  (pm/git "checkout" PCRE2-tag)
  (os/cd "../..")
  (pm/git "submodule" "update" "--init" "--recursive"))

(defn- lib-prefix []
  (if (= (os/which) :windows)
    ""
    "lib"))

(defn- lib-suffix []
  (if (= (os/which) :windows)
    "-static.lib"
    ".a"))

(def- pcre2-lib
  (string (lib-prefix) "pcre2-8" (lib-suffix)))

(def- pcre2-build-dir "_build/pcre2")

(def- cmake-flags @["-B" pcre2-build-dir "-S" "libs/pcre2" "-G" "Ninja"
                    "-DCMAKE_BUILD_TYPE=Release" "-DPCRE2_SUPPORT_JIT=ON"
                    "-DPCRE2_STATIC_PIC=ON" "-DBUILD_SHARED_LIBS=OFF"])
(def- cmake-build-flags @["--build" pcre2-build-dir "--parallel" "--config" "Release"])

(def- pcre2-static-lib
  (if (= (os/which) :windows)
    "pcre2-8-static.lib"
    "libpcre2-8.a"))

(defn- clean-static-lib
  "Remove old static lib from _build directory"
  []
  (print "removing static lib")
  (let [a (path/join "./jre" pcre2-static-lib)]
    (when (sh/exists? a)
      (sh/rm a))))

(defn- copy-static-lib
  "Copy static lib to _build directory for install"
  []
  (print "copying static lib")
  (let [infile (string/format "%s/%s" pcre2-build-dir pcre2-static-lib)
        outfile (string/format "jre/%s" pcre2-static-lib)]
    (when (sh/exists? infile)
      (sh/copy-file infile outfile))))

(defn build-pcre2 []
  (unless (and (sh/exists? "libs/pcre2") (sh/exists? "libs/pcre2/deps/sljit"))
    (update-submodules))
  (clean-static-lib)
  (unless (sh/exists? (string/format "%s/%s" pcre2-build-dir "build.ninja"))
    (cmake ;cmake-flags))
  (do (cmake ;cmake-build-flags))
  (copy-static-lib))

# create new task to build C PCRE2 static lib
(task "build-pcre2" [] (build-pcre2))

# attach this task to run during pre-build
(task "pre-build" ["build-pcre2"])

(defn gen-lflags []
  (if (= (os/which) :windows)
    @[(string/format "/LIBPATH:./%s" pcre2-build-dir) "pcre2-8-static.lib"]
    @[(string/format "-L%s" pcre2-build-dir) "-lpcre2-8"]))

(def cflags @["-I_build/pcre2"])

(declare-source
  :source ["jre"])

(declare-native
  :name "jre/native"
  :source @["cpp/module.cpp"]
  :use-rpath true
  :c++flags cflags
  :lflags (gen-lflags))

(defn- fix-up-ldflags []
  (def jre-dir (path/join (dyn *syspath*) "jre"))
  (def meta-file (path/join jre-dir "native.meta.janet"))
  (unless (sh/exists? meta-file)
    (printf "Unable to find meta file: %s" meta-file)
    (os/exit 1))
  # get the contents of the current meta file
  (def meta-data (slurp meta-file))
  # find the header/comment if it exists
  (var comment "# meta file for jre")
  (let [parts (string/split "\n" meta-data)]
    (when (string/find "#" (parts 0))
      (set comment (parts 0))))
  # parse the current data into a struct
  (def old-meta (parse meta-data))
  # create new lflags that point to the :syspath location
  (def new-lflags @[(string/format (if (= (os/which) :windows) "/LIBPATH:%s" "-L%s") jre-dir)])
  (loop [item :in (old-meta :lflags)]
    (when (string/find "-8" item)
      (array/push new-lflags item)))
  (def new-meta-data @{})
  (loop [key :in (keys old-meta)]
    (if (= key :lflags)
      (set (new-meta-data key) new-lflags)
      (set (new-meta-data key) (old-meta key))))
  (spit meta-file (string/format "%s\n\n%m" comment (table/to-struct new-meta-data))))

# create a new task to run the ldflags fixup
(task "fix-up-ldflags" [] (fix-up-ldflags))

# attach this task to the post-install hook
(task "post-install" ["fix-up-ldflags"])

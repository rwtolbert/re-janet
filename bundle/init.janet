(if (dyn :install-time-syspath)
  (use @install-time-syspath/spork/declare-cc)
  (use spork/declare-cc))

(import spork/pm)
(import spork/sh)
(import spork/path)

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

(defn- cmake
  "Make a call to cmake."
  [& args]
  (printf "cmake %j" args)
  (sh/exec (dyn *cmakepath* "cmake") ;args))

(defn- update-submodules []
  (pm/git "submodule" "update" "--init" "--recursive")
  (pm/git "checkout" "pcre2-10.45")
  (pm/git "submodule" "update" "--init" "--recursive"))

(defn- lib-prefix []
  (if (= (os/which) :windows)
    ""
    "lib"))

(defn- lib-suffix []
  (if (= (os/which) :windows)
    ".lib"
    ".a"))

(def- pcre2-lib
  (string (lib-prefix) "pcre2-8" (lib-suffix)))

(def- pcre2-build-dir "_build/pcre2")

(def- cmake-flags @["-B" pcre2-build-dir "-S" "libs/pcre2" "-G" "Ninja"
                    "-DCMAKE_BUILD_TYPE=Release" "-DPCRE2_SUPPORT_JIT=ON" "-DPCRE2_STATIC_PIC=ON" "-DBUILD_SHARED_LIBS=OFF"])
(def- cmake-build-flags @["--build" pcre2-build-dir "--parallel" "--config" "Release"])

(defn build-pcre2 []
  (unless (and (sh/exists? "libs/pcre2") (sh/exists? "libs/pcre2/deps/sljit"))
    (update-submodules))
  (unless (sh/exists? (string/format "%s/%s" pcre2-build-dir pcre2-lib))
    (unless (sh/exists? (string/format "%s/%s" pcre2-build-dir "build.ninja"))
      (cmake ;cmake-flags))
    (do (cmake ;cmake-build-flags))))

(set-command "cmake" *cmakepath*)
(set-command "ninja" *ninjapath*)

(defn gen-lflags []
  (if (= (os/which) :windows)
    @[(string/format "/LIBPATH:./%s/" pcre2-build-dir) "pcre2-8.lib"]
    @[(string/format "-L%s" pcre2-build-dir) "-lpcre2-8"]))

(defdyn *lflags* "Linker flags")
(setdyn *lflags* (gen-lflags))

(task "pre-build" ["build-pcre2"])

(dofile "project.janet" :env (jpm-shim-env))

(task "build-pcre2" []
      (build-pcre2))

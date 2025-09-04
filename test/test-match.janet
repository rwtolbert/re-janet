(use spork/test)
(import spork/path)

(import jre)

(start-suite 'match)

# try some matches with working regexes
(def pos-int (jre/compile "[0-9]+"))
(assert (jre/match pos-int "123"))

# won't match a neg number
(assert (not (jre/match pos-int "-14")))

# won't match a plain ascii string
(assert (not (jre/match pos-int "hello")))

(def int1 (jre/compile "[-+]?[0-9]+"))
(assert (jre/match int1 "123"))
(assert (jre/match int1 "-3"))
(assert (jre/match int1 "+34"))
(assert (not (jre/match int1 "+34.3")))

# case sensitive match
(def ucase (jre/compile "[A-Z]+"))
(assert (jre/match ucase "HELLO"))
(assert (not (jre/match ucase "hello")))

# case insensitive match
(def anycase (jre/compile "[A-Z]+" jre/:ignorecase))
(assert (jre/match anycase "HELLO"))
(assert (jre/match anycase "hello"))
(assert (jre/match anycase "HeLlO"))

# try match instead of search since we match the whole thing
# as well as using a regex as string instead of pre-compiled
(def explicit-color "([0-5]),([0-5]),([0-5]):(.+)")
(def explicit-input "5,5,0:foobar")
(def results (jre/match explicit-color explicit-input))
(let [[_ r g b str] (seq [x :in (results :groups)] (x :str))]
  (assert (= r "5"))
  (assert (= g "5"))
  (assert (= b "0"))
  (assert (= str "foobar")))

(def dirname-patt (jre/compile (string ".*" "_build/" "$")))
(assert (jre/search dirname-patt "/foo/bar/_build/"))
(assert (jre/search dirname-patt (path/dirname "/foo/bar/_build/foo.obj")))

(def dirname-patt (jre/compile (string "^" "/foo/bar/_build")))
(assert (jre/search dirname-patt "/foo/bar/_build/"))
(assert (jre/search dirname-patt (path/dirname "/foo/bar/_build/foo.obj")))
(assert (jre/search dirname-patt "/foo/bar/_build/foo.obj"))


(end-suite)

(use spork/test)

(import jre)

(start-suite 'search)

(def hello (jre/compile "hello"))
(def hello-bol (jre/compile "^hello"))
(def hello-eol (jre/compile "hello$"))
(def world-eol (jre/compile "world$"))
(def hello-both (jre/compile "^hello$"))

(assert (jre/search hello "hello, world"))
(assert (jre/search hello-bol "hello, world"))
(assert (not (jre/search hello-eol "hello, world")))
(assert (jre/search world-eol "hello, world"))
(assert (not (jre/search hello-both "hello, world")))
(assert (jre/search hello-both "hello"))

(def input "hello world, hello sky, hello moon")
(def hits (jre/search hello input))
# (printf "matches: %q" hits)
(assert (= (length hits) 3))
(seq [[b e] :in hits]
  (assert (= "hello" (string/slice input b e))))

(end-suite)
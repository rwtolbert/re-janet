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
(assert (= (length hits) 3))

(seq [result :in hits]
  (assert (= "hello" (string/slice input (result :begin) (result :end)))))

(def poem "Roses are #ff0000, violets are #0000ff")
(def color-regex "#([a-f0-9]{2})([a-f0-9]{2})([a-f0-9]{2})")
(def colors (jre/search color-regex poem))
(assert (= (length colors) 2))
(let [first-match ["ff" "00" "00"]
      second-match ["00" "00" "ff"]
      first-result (get colors 0)
      second-result (get colors 1)
      [_ r1 g1 b1] (first-result :groups)
      [_ r2 g2 b2] (second-result :groups)]
  (assert (deep= [r1 g1 b1] first-match))
  (assert (deep= [r2 g2 b2] second-match)))

(let [results (jre/replace color-regex poem "******")]
  (assert (deep= (string/find-all "******" results) @[10 30])))

(def explicit-color "^([0-5]),([0-5]),([0-5]):(.+)$")
(def explicit-input "5,5,0:foobar")
(def results (jre/search explicit-color explicit-input))
(assert (= (length results) 1))
(let [res (get results 0)
      [_ r g b str] (res :groups)]
  (assert (= r "5"))
  (assert (= g "5"))
  (assert (= b "0"))
  (assert (= str "foobar")))

# count words
(def word-regex "(\\w+)")
(def words (jre/search word-regex poem))
(assert (= (length words) 6))

(end-suite)
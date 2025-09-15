(use spork/test)
(import spork/path)

(import jre)

(start-suite 'match)

# try some matches with working regexes
(def pos-int (jre/compile "[0-9]+"))
(def m (jre/match pos-int "123"))
(assert (= (length m) 1))
(assert (= ((m 0) :val) "123"))

# test with bad input
(assert-error "should error with first arg not string or regex" (not (jre/match @[] @[])))
(assert-error "should error with second arg not string" (not (jre/match pos-int @[])))

(def m (jre/match pos-int "-14"))
(assert (= (length m) 1))
(assert (= ((m 0) :val) "14"))

# won't match a plain ascii string
(pp (jre/match pos-int "hello"))
(assert (empty? (jre/match pos-int "hello")))

(def int1 (jre/compile "^[-+]?[0-9]+$"))
(assert (jre/match int1 "123"))
(assert (jre/match int1 "-3"))
(assert (jre/match int1 "+34"))
# should fail to match a float with the '.'
(assert (empty? (jre/match int1 "+34.3")))

# case sensitive match
(def ucase (jre/compile "[A-Z]+" :std))
(pp (jre/match ucase "HELLO"))
(assert (jre/match ucase "HELLO"))
(assert (empty? (jre/match ucase "hello")))

# case insensitive match
(def anycase (jre/compile "[A-Z]+" :ignorecase))
(assert (jre/match anycase "HELLO"))
(assert (jre/match anycase "hello"))
(assert (jre/match anycase "HeLlO"))

(defn- test-capture-groups [style]
  # as well as using a regex as string instead of pre-compiled
  (printf "\nTesting captures-> %j" style)
  (def explicit-color (jre/compile "([0-5]),([0-5]),([0-5]):(([a-z])[a-z]+)" style))
  (pp explicit-color)
  (def explicit-input "5,5,0:foo 4,4,3:bar")
  (pp (jre/find-all explicit-color explicit-input))
  (def results (jre/match explicit-color explicit-input))
  (pp results)
  (assert (= (length results) 2))
  # outer :str is entire match
  # rest are captures 1,2,3,4
  (pp ((results 0) :val))
  (assert (= ((results 0) :val) "5,5,0:foo"))
  #(each item ((results 0) :groups)
  #  (pp item))
  (assert (= ((((results 0) :groups) 0) :val) "5"))
  (assert (= ((((results 0) :groups) 1) :val) "5"))
  (assert (= ((((results 0) :groups) 2) :val) "0"))
  (assert (= ((((results 0) :groups) 3) :val) "foo")))

(test-capture-groups :std)
(test-capture-groups :pcre2)

(defn- full? [cont]
  (not (empty? cont)))

(defn- test-begin-end [style]
  (def hello (jre/compile "hello" style))
  (def hello-bol (jre/compile "^hello" style))
  (def hello-eol (jre/compile "hello$" style))
  (def world-eol (jre/compile "world$" style))
  (def hello-both (jre/compile "^hello$" style))

  (assert (full? (jre/match hello "hello, world")))
  (assert (full? (jre/match hello-bol "hello, world")))
  (assert (empty? (jre/match hello-eol "hello, world")))
  (assert (full? (jre/match world-eol "hello, world")))
  (assert (empty? (jre/match hello-both "hello, world")))
  (assert (full? (jre/match hello-both "hello")))

  (def input "hello world, hello sky, hello moon")
  (def hits (jre/match hello input))
  (pp hits)
  (assert (= (length hits) 3)))

(test-begin-end :std)
(test-begin-end :pcre2)

(def poem "Roses are #ff0000, violets are #0000ff")

(defn- test-poem [style]
  (printf "\nTesting poem %j" style)
  (def color-regex (jre/compile "#([a-f0-9]{2})([a-f0-9]{2})([a-f0-9]{2})" style))
  (def colors (jre/match color-regex poem))
  (printf "%Q" colors)
  (assert (= (length colors) 2))
  (let [first-match ["ff" "00" "00"]
        second-match ["00" "00" "ff"]
        first-result (get colors 0)
        second-result (get colors 1)
        [r1 g1 b1] (seq [x :in (first-result :groups)] (x :val))
        [r2 g2 b2] (seq [x :in (second-result :groups)] (x :val))]
    (assert (deep= [r1 g1 b1] first-match))
    (assert (deep= [r2 g2 b2] second-match)))

  #(let [results (jre/replace-all color-regex poem "******")]
  #  (assert (deep= (string/find-all "******" results) @[10 30])))

  # count words
  (def word-regex (jre/compile "(\\w+)" style))
  (def words (jre/match word-regex poem))
  (assert (= (length words) 6)))

(test-poem :std)
(test-poem :pcre2)

(defn- test-other-captures [style]
  (printf "\nTest other captures %j" style)
  # how to handle non-capturing groups in the regex
  (def code "(def [*bool* *no-bool*] )")
  (def patt (jre/compile "(?:^|[^*])([*][-a-zA-Z0-9]+[*])(?=$|[^*])" style))
  (def earmuffs (jre/match patt code))
  (assert (= (length earmuffs) 2))
  (assert (= ((((earmuffs 0) :groups) 0) :val) "*bool*"))
  (assert (= ((((earmuffs 1) :groups) 0) :val) "*no-bool*"))
  #(printf "%Q" earmuffs)

  (def line "foo bar baz 123")
  (def finder (jre/compile "(foo)|(bar)|([0-9]+)" style))
  (def foobar (jre/match finder line))
  # should get three matches
  (assert (= (length foobar) 3))
  # first match hits first group, 2nd hits 2nd, 3rd hits third
  (pp foobar)
  (eachp [i m] foobar
    (assert (= (length (m :groups)) 1))
    (each grp (m :groups)
      (assert (= (grp :group-index) (+ i 1))))))

(test-other-captures :std)
(test-other-captures :pcre2)

(end-suite)

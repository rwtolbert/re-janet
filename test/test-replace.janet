(use spork/test)

(import jre)

(start-suite 'replace)

(def hello (jre/compile "hello"))

(def input "hello world, hello sky, hello moon")

# replacement with a pre-compiled regex
(def replaced (jre/replace hello input "goodbye"))
(assert replaced)
(printf "replaced: %q" replaced)

# replacement with a string that is regex-ed first
(def sentence "Quick brown fox")
(def pattern "a|e|i|o|u")
(def after (jre/replace-all pattern sentence "[$&]"))
(assert after)
(printf "after: %q" after)

(def first-only (jre/replace "hello" input "goodbye"))
(printf "first-only: %q" first-only)
(assert (jre/contains? "hello" first-only))
(assert (jre/contains? "goodbye" first-only))

# vowel replace
(defn vowel-replace [style]
  (def patt (jre/compile "a|e|i|o|u" style))
  (def after (jre/replace pattern sentence "[$&]"))
  (assert after)
  (printf "%j after: %q" style after)
  (assert (= (length (string/find-all "[" after)) 1))

  (def after (jre/replace-all pattern sentence "[$&]"))
  (assert after)
  (printf "%j after: %q" style after)
  (assert (= (length (string/find-all "[" after)) 4)))

(vowel-replace :std)
(vowel-replace :pcre2)

(end-suite)

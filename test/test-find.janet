(use spork/test)
(import spork/path)

(import jre)

(start-suite 'find)

# PCRE2
(def pcre2-pos-int (jre/pcre2-compile "[0-9]+"))
(assert (= 0 (jre/pcre2-find pcre2-pos-int "123 asdfih asdf")))
(assert (= 1 (jre/pcre2-find pcre2-pos-int "-14")))
(assert (= 5 (jre/pcre2-find pcre2-pos-int "abcd 12 def 14")))
(assert (not (jre/pcre2-find pcre2-pos-int "abc")))

(def all-results (jre/pcre2-findall pcre2-pos-int "123 asd456 as78"))
(pp all-results)
(assert (= 3 (length all-results)))

# single
(assert (= 8 (length (jre/pcre2-findall "[0-9]" "123 asd456 as78"))))


# PCRE2 match
(pp (jre/pcre2-match pcre2-pos-int "123 asd456 as78"))

(end-suite)

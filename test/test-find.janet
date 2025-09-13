(use spork/test)
(import spork/path)

(import jre)

(start-suite 'find)

# STD
(def pos-int (jre/compile "[0-9]+" :std))
(assert (= 0 (jre/find pos-int "123 asdfih asdf")))
(assert (= 1 (jre/find pos-int "-14")))
(assert (= 5 (jre/find pos-int "abcd 12 def 14")))

# check start-index
(assert (= 12 (jre/find pos-int "abcd 12 def 14" 7)))

(def all-results (jre/find-all pos-int "123 asd456 as78"))
(pp all-results)
(assert (= 3 (length all-results)))

# find-all with start-index
(def all-results (jre/find-all pos-int "123 asd456 as78" 4))
(pp all-results)
(assert (= 2 (length all-results)))

# PCRE2
(def pcre2-pos-int (jre/compile "[0-9]+"))
(assert (= 0 (jre/find pcre2-pos-int "123 asdfih asdf")))
(assert (= 1 (jre/find pcre2-pos-int "-14")))
(assert (= 5 (jre/find pcre2-pos-int "abcd 12 def 14")))
(assert (not (jre/find pcre2-pos-int "abc")))

# check start-index
(assert (= 12 (jre/find pcre2-pos-int "abcd 12 def 14" 7)))

(def all-results (jre/find-all pcre2-pos-int "123 asd456 as78"))
(pp all-results)
(assert (= 3 (length all-results)))

# find-all with start-index
(def all-results (jre/find-all pcre2-pos-int "123 asd456 as78" 8))
(pp all-results)
(assert (= 2 (length all-results)))

# single number
(assert (= 8 (length (jre/find-all "[0-9]" "123 asd456 as78"))))

(end-suite)

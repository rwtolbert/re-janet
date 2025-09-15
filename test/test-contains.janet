(use spork/test)
(import spork/path)

(import jre)

(start-suite 'contains)

# try some contains with working regexes
(def pos-int (jre/compile "[0-9]+" :std))
(assert (jre/contains? pos-int "123 asdfih asdf"))
(assert (jre/contains? pos-int "-14"))
(assert (not (jre/contains? pos-int "abc")))

# test with bad input
(assert-error "should error with first arg not string or regex" (not (jre/contains? @[] @[])))
(assert-error "should error with second arg not string" (not (jre/contains? pos-int @[])))

# PCRE2
(def pcre2-pos-int (jre/compile "[0-9]+" :pcre2))
(assert (jre/contains? pcre2-pos-int "123 asdfih asdf"))
(assert (jre/contains? pcre2-pos-int "-14"))
(assert (not (jre/contains? pcre2-pos-int "abc")))

# string -> PCRE2
(assert (jre/contains? "[0-9]+" "123 asdfih asdf"))
(assert (jre/contains? "[0-9]+" "-14"))
(assert (not (jre/contains? "[0-9]+" "abc")))

(end-suite)

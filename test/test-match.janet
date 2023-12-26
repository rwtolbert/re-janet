(use spork/test)

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

(end-suite)
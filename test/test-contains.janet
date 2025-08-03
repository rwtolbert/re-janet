(use spork/test)
(import spork/path)

(import jre)

(start-suite 'contains)

# try some contains with working regexes
(def pos-int (jre/compile "[0-9]+"))
(assert (jre/contains pos-int "123 asdfih asdf"))
(assert (jre/contains pos-int "-14"))
(assert (not (jre/contains pos-int "abc")))

(end-suite)
(import jre)
(use spork/test)

(start-suite 'jre)

(assert (= (jre/hello-native) "Hello!"))

# make sure we get an error when trying to create an invalid regex
(assert-error "should error with not a keyword" (jre/compile "(\\w+)" "foo"))
(assert-error "should error with bad keyword" (jre/compile "(\\w+)" :fooasdfsad))
(assert-error "can't use two different styles of match" (jre/compile "(\\w+)" jre/:basic jre/:awk))
(assert-error "bad regex pattern" (jre/compile "(\\w+"))

(def matcher (jre/compile "(\\w+)" jre/:ignorecase jre/:optimize))
(printf "matcher: %q" matcher)
(assert (jre/match matcher "hello"))

# try some matches with working regexes
(def pos-int (jre/compile "[0-9]+"))
(assert (jre/match pos-int "123"))
(assert (not (jre/match pos-int "hello")))

(def ucase (jre/compile "[A-Z]+"))
(assert (jre/match ucase "HELLO"))
(assert (not (jre/match ucase "hello")))

# multiline test
(def world (jre/compile "^world$" :multiline))
(assert (jre/match world "world"))

(end-suite)

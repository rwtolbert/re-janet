(use spork/test)

(import jre)

(start-suite 'compile)

# make sure we get an error when trying to create an invalid regex
(assert-error "should error with not a keyword" (jre/compile "(\\w+)" "foo"))
(assert-error "should error with bad keyword" (jre/compile "(\\w+)" :fooasdfsad))
(assert-error "can't use two different styles of match" (jre/compile "(\\w+)" jre/:basic jre/:awk))
(assert-error "bad regex pattern" (jre/compile "(\\w+"))
(assert-error "mismatched []" (jre/compile "([.)"))

(assert-no-error "test compile with flag"
                 (def matcher (jre/compile "(\\w+)" jre/:ignorecase)))
(assert-no-error "test compile with multiple flags"
                 (def matcher (jre/compile "(\\w+)" jre/:ignorecase jre/:optimize)))

(end-suite)

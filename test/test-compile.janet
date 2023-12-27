(use spork/test)

(import jre)

(start-suite 'compile)

# make sure we get an error when trying to create an invalid regex
(assert-error "should error with not a keyword" (jre/compile "(\\w+)" "foo"))
(assert-error "should error with bad keyword" (jre/compile "(\\w+)" :fooasdfsad))
(assert-error "can't use two different styles of match" (jre/compile "(\\w+)" jre/:basic jre/:awk))
(assert-error "bad regex pattern" (jre/compile "(\\w+"))

(assert-no-error "test compile with multiple flags"
                 (def matcher (jre/compile "(\\w+)" jre/:ignorecase jre/:optimize)))

# make sure error is the right error - might be c++ implementation-dependent
(try
  (jre/compile "([.)")
  ([err]
   (assert (string/find "mismatched [ and ]" err))))

(end-suite)
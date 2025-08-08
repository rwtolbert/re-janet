(use spork/test)

(import jre)

(start-suite 'compile)

(defn- contains [patt str]
  return (not (nil? (string/find patt str))))

(defmacro check-error [body msg]
  ~(try
     ,body
     ([err fib] (do (pp err)
                  (assert (contains ,msg err))))))

# make sure we get an error when trying to create an invalid regex

(assert-error "should error with not a keyword" (jre/compile "(\\w+)" "foo"))
(assert-error "should error with bad keyword" (jre/compile "(\\w+)" :fooasdfsad))
(assert-error "can't use two different styles of match" (jre/compile "(\\w+)" jre/:basic jre/:awk))
(assert-error "bad regex pattern" (jre/compile "(\\w+"))
(assert-error "mismatched []" (jre/compile "([.)"))

# check error messages
(check-error (jre/compile "(\\w+)" "foo") "Regex flags must be keyword from")
(check-error (jre/compile "(\\w+)" :fooasdfsad) "is not a valid regex flag")

(assert-no-error "test compile with multiple flags"
                 (def matcher (jre/compile "(\\w+)" jre/:ignorecase jre/:optimize)))


(assert-no-error "test PCRE2 compile"
                 (def matcher (jre/pcre2-compile "(\\w+)")))

(assert-error "bad regex pattern" (jre/pcre2-compile "(\\w+"))
(assert-error "mismatched []" (jre/pcre2-compile "([.)"))

(check-error (jre/pcre2-compile "(\\w+") "PCRE2 compilation failed")
(check-error (jre/pcre2-compile "([.)") "PCRE2 compilation failed")

(end-suite)

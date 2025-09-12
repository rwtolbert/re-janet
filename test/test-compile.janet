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

(assert-error "should error with not a keyword" (jre/compile "(\\w+)" :std "foo"))
(assert-error "should error with bad keyword" (jre/compile "(\\w+)" :std :fooasdfsad))
(assert-error "can't use two different styles of match" (jre/compile "(\\w+)" :std :basic :awk))
(assert-error "bad regex pattern" (jre/compile "(\\w+" :std))
(assert-error "mismatched []" (jre/compile "([.)" :std))

# check error messages
(check-error (jre/compile "(\\w+)" :std "foo") "C++ std::regex flags must be keyword from")
(check-error (jre/compile "(\\w+)" :std :fooasdfsad) "is not a valid C++ std::regex flag")

(pp (jre/compile "(\\w+)" :ignorecase))
(pp (jre/compile "(\\w+)" :std :ignorecase))
(pp (jre/compile "(\\w+)" :pcre2 :ignorecase))

(assert-no-error "test compile with multiple flags"
                 (def matcher (jre/compile "(\\w+)" :std :ignorecase :optimize)))

(assert-no-error "test PCRE2 compile"
                 (def matcher (jre/compile "(\\w+)" :pcre2)))

(assert-error "bad regex pattern" (jre/compile "(\\w+"))
(assert-error "mismatched []" (jre/compile "([.)"))

(check-error (jre/compile "(\\w+)" :pcre2 :basic) "is not a valid PCRE2 regex flag")
(check-error (jre/compile "(\\w+)" :basic) "is not a valid PCRE2 regex flag")
(check-error (jre/compile "(\\w+)" :pcre2 :fooasdfsad) "is not a valid PCRE2 regex flag")

(check-error (jre/compile "(\\w+") "PCRE2 compilation failed")
(check-error (jre/compile "([.)") "PCRE2 compilation failed")

(end-suite)

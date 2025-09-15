(import jre/native :export true :prefix "_")

(defn compile
  ```Compile regex for repeated use.

Flags let you control the syntax and contents of the regex.

The engine is chosen based on one of these flags:

* :pcre2 - Use PCRE2 engine [default] (https://www.pcre.org/)
* :std - Use C++ std::regex based on C++ std::regex rules.
   (https://en.cppreference.com/w/cpp/regex/syntax_option_type)

The following options are available for both engines:

* :ignorecase - use case-insensitive matching

Options for C++ std::regex:

* :optimize - optimize regex for matching speed
* :collate - character ranges are locale sensitive

C++ std::regex grammar options: (These are mutually exclusive)

* :ecmascript - Modified ECMAScript grammar (https://en.cppreference.com/w/cpp/regex/ecmascript)
* :basic - basic POSIX regex grammar
* :extended - extended POSIX regex grammar
* :awk - POSIX awk regex grammar
* :grep - POSIX grep regex grammar
* :egrep - POSIX egrep regex grammar
  ```
  [regex & flags]
  (var use-pcre2 true)
  (let [cf @[]]
    (each item flags
      (cond
        (= item :pcre2) (set use-pcre2 true)
        (= item :std) (set use-pcre2 false)
        true (array/push cf item)))
    (if use-pcre2
      (_pcre2-compile regex ;cf)
      (_std-compile regex ;cf))))

(defn contains?
  ```Return true if `patt` is somewhere in `text`.

`patt` can be a regex string or precompiled with `jre/compile`.
```
  [patt text]
  (if (or (string? patt) (= (type patt) :pcre2))
    (_pcre2-contains patt text)
    (_std-contains patt text)))

(defn find
  ```Return position of first match of `patt` in `text`. Returns nil
if not found.

`patt` can be a regex string or precompiled with `jre/compile`.
```
  [patt text &opt start-index]
  (default start-index 0)
  (if (or (string? patt) (= (type patt) :pcre2))
    (_pcre2-find patt text start-index)
    (_std-find patt text start-index)))

(defn find-all
  ```Return array of all positions of `patt` in `text`.

`patt` can be a regex string or precompiled with `jre/compile`.
```
  [patt text &opt start-index]
  (default start-index 0)
  (if (or (string? patt) (= (type patt) :pcre2))
    (_pcre2-find-all patt text start-index)
    (_std-find-all patt text start-index)))


(defn match
  ```Return array of captures of `patt` in `text`. Return `nil`
if no match is found.

`patt` can be a regex string or precompiled with `jre/compile`.
```
  [patt text &opt start-index]
  (default start-index 0)
  (if (or (string? patt) (= (type patt) :pcre2))
    (_pcre2-match patt text start-index)
    (_std-match patt text start-index)))

(defn replace
  ```Replace first occurrence of `patt` in `text` with `subst`.

`patt` can be a regex string or precompiled with `jre/compile`.
```
  [patt text subst]
  (if (or (string? patt) (= (type patt) :pcre2))
    (_pcre2-replace patt text subst)
    (_std-replace patt text subst)))

(defn replace-all
  ```Replace all occurrences of `patt` in `text` with `subst.

`patt` can be a regex string or precompiled with `jre/compile`.
```
  [patt text subst]
  (if (or (string? patt) (= (type patt) :pcre2))
    (_pcre2-replace-all patt text subst)
    (_std-replace-all patt text subst)))

(defn regex-split
  ```Split `text` on `patt` returning array of parts```
  [patt text]
  (let [results (match patt text)
        parts @[]]
    (if (empty? results)
      @[text]
      (do
        (var start 0)
        (seq [part :in results]
          (array/push parts (string/slice text start (part :begin)))
          (set start (part :end)))
        parts))))

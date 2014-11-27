;; I'm debating to myself if I should consider these tests as
;; describing normative behavior of Arc, or quirks of MzScheme,
;; and thus onions in the varnish.
(register-test '(suite "Foundation Tests"
  (suite "Symbols"
    (suite "piped"
      (suite "plain"
        ("write the value of a plain symbol enclosed in pipes"
          (tostring:write '|foo|)
          "foo")

        ("disp the value of a plain symbol enclosed in pipes"
          (tostring:disp '|foo|)
          "foo"))

      (suite "containing #\\;"
        ("write '|foo;bar|"
          (tostring:write '|foo;bar|)
          "|foo;bar|")

        ("disp '|foo;bar|"
          (tostring:disp '|foo;bar|)
          "foo;bar"))

      (suite "containing #\\newline"
        ("write '|foo\\nbar|"
          (tostring:write (coerce "foo\nbar" 'sym))
          "|foo
bar|")

        ("disp '|foo\\nbar|"
          (tostring:disp (coerce "foo\nbar" 'sym))
          "foo
bar"))

      (suite "empty"
        ("write '||"
          (tostring:write '||)
          "||")

        ("disp '||"
          (tostring:disp '||)
          ""))

      (suite "dot"
        ("write '|.|"
          (tostring:write '|.|)
          "|.|")

        ("disp '|.|"
          (tostring:disp '|.|)
          "."))

      (suite "numbers and special chars need piping"
        ("21, for example"
          (coerce "21" 'sym)
          |21|)

        ("1.23E10"
          (coerce "1.23E10" 'sym)
          |1.23E10|)

        (";"
          (coerce ";" 'sym)
          |;|)

        ("NaN"
          (coerce "+nan.0" 'sym)
          |+nan.0|))

      (suite "it's not over yet"
        ("show sym with pipes"
          (coerce "|foo|" 'sym)
          \|foo\|)

        ("show sym with parens"
          (coerce "(foo)" 'sym)
          |(foo)|)

        ("write sym containing pipes"
          (tostring:write (coerce "|foo|" 'sym))
          "\\|foo\\|")

        ("disp sym containing pipes"
          (tostring:disp (coerce "|foo|" 'sym))
          "|foo|")

        ("sym containing double-quotes"
          (coerce "\"foo\"" 'sym)
          |"foo"|)

        ("sym containing single-quotes"
          (coerce "'" 'sym)
          |'|)

        ("let's go wild"
          (coerce "||||||||||||||" 'sym)
          \|\|\|\|\|\|\|\|\|\|\|\|\|\|)
      )

      (suite "displaying"
        ("empty symbol"
          (coerce (coerce "" 'sym) 'string)
          ""))))))

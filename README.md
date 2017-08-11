# Poi

A dynamically typed pure functional programming language.

[![Build Status](https://travis-ci.org/stepchowfun/poi.svg?branch=master)](https://travis-ci.org/stepchowfun/poi)

## Installation

    curl -sSf https://raw.githubusercontent.com/stepchowfun/poi/master/scripts/install.sh | sh

## Syntax

Comments:

    # This is a comment.

Functions:

    x -> x

Application:

    x -> x x

Bindings:

    id = x -> x
    id

Algebraic data types and pattern matching:

    bool = data {true, false}
    not = x -> match x {
      {true} -> bool.false
      {false} -> bool.true
    }

    option = data {none, some value}
    map = f -> x -> match x {
      {none} -> option.none
      {some value} -> option.some (f value)
    }

    map not (option.some bool.false)

Grouping:

    f -> g -> x -> f (g x)

Line continuations:

    f -> g -> x -> \
      f (g x)

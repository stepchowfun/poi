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

Let bindings:

    id = x -> x
    id

Algebraic data types:

    bool = data (true, false)
    true = bool.true
    false = bool.false

    option = data (none, some value)

    something = option.some true
    something.value

Pattern matching:

    option = data (none, some value)
    meal = option.some (option.some (x -> x))
    {some burrito} = meal

    f = {some {some z}} -> z
    f meal

Grouping:

    f -> g -> x -> f (g x)

Line continuations:

    f -> g -> x -> \
      f (g x)

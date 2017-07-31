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

    maybe = data (none, just value)

    tree = data (
      empty
      node left value right
    )

    subtree = tree.node tree.empty true tree.empty
    tree.node subtree false subtree

Grouping:

    f -> g -> x -> f (g x)

Line continuations:

    f -> g -> x -> \
      f (g x)

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

Grouping:

    f -> g -> x -> f (g x)

Line continuations:

    f -> g -> x -> \
      f (g x)

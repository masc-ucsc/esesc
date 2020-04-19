# ESESC and GitHub

ESESC is an architectural simulator that is primarily maintained and developed
by the [MASC lab][masc] at UC Santa Cruz.  Since ESESC is used for computer
architecture research, the [MASC lab][masc] does development using a private
repo so that we can wait until the research is published before pushing changes
to the public repo hosted on GitHub.

This document describes the technique used at by the [MASC lab][masc] for maintaining
a private ESESC repo and integrating changes with the public repo
that is hosted on GitHub.  Other groups may choose to adapt this
technique for their own use.

## Public ESESC GitHub Repo

If you do not need the private repo, just get the public repo by executing:

    git clone https://github.com/masc-ucsc/esesc

## Private ESESC MASC Lab Repo

If you are working on ESESC at UC Santa Cruz, contact [Jose Renau](http://users.soe.ucsc.edu/~renau/)
to get access to the private ESESC repo used by the MASC lab. The clone the private ESESC Repo:

    git clone git@github.com:masc-ucsc/esesc-private.git esesc

If you are not tasked with synchronizing your work with the public repo then
you can simply push/pull changes to/from the private repo and someone else
will push them to the public one.

## Synchronizing Public and Private Repos

This section describes how we synchronize the public and private ESESC repos.
Most users can ignore it and simply work on the appropriate public or private 
repo.

### Create Private ESESC repo for first time

If you work outside UCSC, you should clone the public esesc repo. First, create
an private/public empty repository (esesc-private), then run this to close esesc

    # First
    git clone --bare https://github.com/masc-ucsc/esesc
    cd esesc.git
    git push --mirror https://github.com/yourname/esesc-private.git
    cd ..
    rm -rf esesc.git


### Typical usage

The workflow in the [MASC lab][masc] is as follows:

Work in the esesc-masc repo, and commit to master branch

    git clone https://github.com/yourname/esesc-private.git esesc
    cd esesc
    make some changes
    git commit
    git push origin master


To pull latest version of code from esesc public repository

    cd esesc
    git remote add public https://github.com/masc-ucsc/esesc
    git pull public master # Creates a merge commit
    git push origin master

To push your edits to the main public esesc repo (replace XXX by your github name)

    git clone https://github.com/masc-ucsc/esesc esesc-public
    cd esesc-public
    git remote add esesc-private git@github.com:masc-ucsc/esesc-masc.git  # Replace for your private repo
    git checkout -b pull_request_XXX
    git pull esesc-private master
    git push origin pull_request_XXX

Now create a [pull][pull] request through github, and the UCSC/MASC team will review it.


[pull]: https://help.github.com/articles/creating-a-pull-request
[masc]: http://masc.soe.ucsc.edu/

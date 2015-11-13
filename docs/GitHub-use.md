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

    git clone gitosis@mada0.cse.ucsc.edu:esesc.git

If you are not tasked with synchronizing your work with the public repo then
you can simply push/pull changes to/from the private repo and someone else
will push them to the public one.

## Synchronizing Public and Prviate Repos 

This section describes how we synchronize the public and private ESESC repos.
Most users can ignore it and simply work on the appropriate public or private 
repo.  

### Workflow

The workflow in the [MASC lab][masc] is as follows:

1. Commit changes to the `master` branch.

2. Merge the `master` branch with a branch called `github`.  This allows git to merge
the branches correctly without pushing intermediate work to the public repo.

3. Merge `github` with `github_master` using `--squash` to summarize intermediate commits.

### Configuration Steps

Modify your `.git/config` file to contain the following configuration:

    [core]
      repositoryformatversion = 0
      filemode = true
      bare = false
      logallrefupdates = true
    [remote "origin"]
      url = gitosis@mada0.cse.ucsc.edu:esesc.git
      fetch = +refs/heads/*:refs/remotes/origin/*
    [remote "github"]
      url = https://github.com/masc-ucsc/esesc.git
      fetch = +refs/heads/master:refs/remotes/github/github_master
    [branch "master"]
      remote = origin
      merge = refs/heads/master
    [branch "github"]
      remote = origin
      merge = refs/heads/github
    [branch "github_master"]
      remote = github 
      merge  = refs/heads/master
    [push]
      default = upstream

The most important thing to note about this configuration are the sections:

    [remote "github"]
      url = https://github.com/masc-ucsc/esesc.git
      fetch = +refs/heads/master:refs/remotes/origin/github_master
    [branch "github_master"]
      remote = github 

Here we have configured a remote named `github` and a branch named
`github_master`. We will synchronize the branch `github_master` with the
`master` branch in the public GitHub repo.  The refspec allows us to synchronize
the `master` branch on the GitHub repo with a branch called `github_master` that
is in our private repo.

After updating the config file execute the following commands to fetch data from the new branches

    #Save the current git config
    cp .git/config ~/tmp/config

    git fetch
    git fetch github
    git checkout github
    git fetch
    git checkout github_master
    git fetch
    WARNING: Now, the git/config may be different, copy the saved version back
    cp ~/tmp/config .git/config

At this point the repo is configured.  There are two main tasks that you might
want to perform.  One is pushing changes to the remote repo.  The other is
pulling changes from the remote

### Pushing Changes to Public GitHub Repo

Assuming that you have changes merged in to the `master` branch in the private that you want
to push to the public GitHub repo execute the following commands:

    git checkout github
    git merge master
    git checkout github_master
    git merge --squash github
    git commit -m"commit message"

If you have any merge conflicts after the merge you should check and fix them
first.  Then to push the merged changes to the public GitHub repo run:

    #NOTE: if git push fails, do "git pull" to get head of branch
    git push github
    Username for 'https://github.com': <your_username>
    Password for 'https://<your_username>@github.com': 

Note that you can [configure GitHub](https://help.github.com/articles/generating-ssh-keys)
to use SSH keys if you want instead of https.

Also push the changes to the `github_master` branch back to the private repo by
executing:

    #OPTIONAL: just copy github_master to the mada repo (in case that github goes bankrupt ??)
    #git push origin github_master


### Pulling changes from GitHub

*Note this is less tested right now.  These steps will be updated once we have
more collaboration and need to use it.  For now all contributers have been
working on the private repo*

    git checkout github_master
    git pull
    git checkout github
    git merge github_master

Note only execute the following command if the master and github branches
should actually be synchronized.  If there is work on the master branch that
has diverged from the public branch then you may need to apply a patch instead.

    git checkout master
    git merge github
    git push

[masc]: http://masc.soe.ucsc.edu/

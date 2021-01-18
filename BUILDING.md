# Building esesc

## Linux

### Debian
Install the necessary dependencies.
```bash
sudo apt install ncurses pixman-1 bison flex \
                 cmake libpixman-1-dev ncurses

```

## Nix

[Nix](https://nixos.org/) is a way to create reproducible builds
and deployments.

> You can use Nix on any Linux/Darwin distribution or even it's own
> distribution NixOS

A build (_default.nix_) is included, and you can easily install **esesc**
via running `nix build` within the directory or install it by doing:
```bash
nix run -f https://github.com/masc-ucsc/esesc/archive/master.tar.gz --command esesc
```
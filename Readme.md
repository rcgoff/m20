<h1 align="center">Digital Computer M-20 Emulator</h1>
<p align="center">Accurate emulator for soviet general-purpose mainframe M-20</p>
<p align="center">
    <a href="https://github.com/rcgoff/m20/actions/workflows/m20-ci.yml"><img src="https://github.com/rcgoff/m20/actions/workflows/m20-ci.yml/badge.svg" alt="M-20 BUILD"></a>
    <a href="LICENSE"><img src="https://img.shields.io/github/license/rcgoff/m20.svg" alt="License"></a> 
</p>

Languages: [:ru: Русский](./Readme.rus.md) | [:us: English](./Readme.md)

## Building

### Linux

Go to the sources directory:

```shell
cd sources/emulator
```

Build project:

```shell
make -f makefile.unx
```

Run tests:

```shell
make -f makefile.unx test
```

Clean project:

```shell
make -f makefile.unx clean
```

### Windows


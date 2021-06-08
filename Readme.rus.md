<h1 align="center">
Эмулятор ЭЦВМ М-20</h1>
<p align="center">Эмулятор для универсальной автоматической быстроедйствующей цифровой вычислительной машины M20.</p> 
<p align="center">
    <a href="https://github.com/rcgoff/m20/actions/workflows/m20-ci.yml"><img src="https://github.com/rcgoff/m20/actions/workflows/m20-ci.yml/badge.svg" alt="M-20 BUILD"></a>
    <a href="LICENSE"><img src="https://img.shields.io/github/license/rcgoff/m20.svg" alt="License"></a> 
</p>

Языки: [:ru: Русский](./Readme.rus.md) | [:us: English](./Readme.md)

## Сборка

### Linux

Перейти в папку с исходниками:

```shell
cd sources/emulator
```

Собрать исполняемые файлы:

```shell
make -f makefile.unx
```

Запустить тесты:

```shell
make -f makefile.unx test
```

Очистить проект:

```shell
make -f makefile.unx clean
```

### Windows

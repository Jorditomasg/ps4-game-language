<p align="center"><img src="docs/banner.svg" alt="Game Language" width="100%"></p>

<p align="center"><a href="#english">English</a> · <a href="#español">Español</a></p>

<p align="center"><img src="docs/app-preview.png" alt="Game Language app (mock-up)" width="90%"></p>

## English

Choose the language PS4 games start in, by the game's region. Needs [GoldHEN](https://github.com/GoldHEN/GoldHEN).

PS4 games from Europe usually include *Español (España)* and games from America *Español (Latinoamérica)*. If your console uses the other one, the game starts in English. Game Language fixes that.

1. Install `GameLanguage.pkg` from the [latest release](https://github.com/Jorditomasg/ps4-game-language/releases/latest).
2. Open **Game Language**. It installs its GoldHEN plugin by itself.
3. Choose a language for each region: **Automatic**, **System language** or any language. Your system language is shown in green.

Changes apply the next time a game starts. The game must include the chosen language.

## Español

Elige el idioma con el que arrancan los juegos de PS4 según la región del juego. Necesita [GoldHEN](https://github.com/GoldHEN/GoldHEN).

Los juegos europeos suelen traer *Español (España)* y los americanos *Español (Latinoamérica)*. Si la consola está en el otro, el juego arranca en inglés. Game Language lo soluciona.

1. Instala `GameLanguage.pkg` de la [última versión](https://github.com/Jorditomasg/ps4-game-language/releases/latest).
2. Abre **Game Language**. Instala su plugin de GoldHEN automáticamente.
3. Elige un idioma para cada región: **Automático**, **Idioma del sistema** o cualquier idioma. El idioma del sistema sale en verde.

Los cambios se aplican la próxima vez que abras un juego. El juego tiene que incluir el idioma elegido.

## Build

```sh
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis/PS4Toolchain GOLDHEN_SDK=/path/to/GoldHEN_Plugins_SDK
make        # app/IV0000-GLNG00001_00-GAMELANGUAGE0000.pkg
make test
```

Credits: [GoldHEN Plugins SDK](https://github.com/GoldHEN/GoldHEN_Plugins_SDK), [OpenOrbis](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain), [stb_truetype](https://github.com/nothings/stb), [Noto Sans](app/assets/fonts/OFL.txt). License: [GPL-3.0](LICENSE).

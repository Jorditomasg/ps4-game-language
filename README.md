<p align="center"><img src="docs/banner.svg" alt="Game Language" width="100%"></p>

<p align="center">
  <a href="https://github.com/Jorditomasg/ps4-game-language/actions/workflows/build.yml"><img src="https://github.com/Jorditomasg/ps4-game-language/actions/workflows/build.yml/badge.svg" alt="build"></a>
  <a href="https://github.com/Jorditomasg/ps4-game-language/releases/latest"><img src="https://img.shields.io/github/v/release/Jorditomasg/ps4-game-language" alt="release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/Jorditomasg/ps4-game-language" alt="license"></a>
</p>

<p align="center"><b>English</b> · <a href="README.es.md">Español</a></p>

**Game Language** lets you choose the language of each game on a PS4 with [GoldHEN](https://github.com/GoldHEN/GoldHEN), without changing the console's system language.

The PS4 treats regional variants as separate languages: *Español (España)* and *Español (Latinoamérica)*, *Português (Portugal)* and *Português (Brasil)*, *Français* and *Français (Canada)*. European releases usually include only the European variant and American releases only the American one. If the console uses the other variant, the game starts in **English**. Game Language handles this automatically, and lets you pick any language for any game.

<p align="center"><img src="docs/app-preview.png" alt="Game Language app (mock-up)" width="90%"><br><sub>Mock-up of the app with example games.</sub></p>

## Install

1. Download `GameLanguage.pkg` from the [latest release](https://github.com/Jorditomasg/ps4-game-language/releases/latest).
2. Install it like any other package (for example, from a USB drive with GoldHEN's *Package Installer*).
3. Open **Game Language** from the home screen.

When the app opens, it installs or updates the plugin in `/data/GoldHEN/plugins/` and adds it to `/data/GoldHEN/plugins.ini`. Before changing `plugins.ini`, it saves a backup as `plugins.ini.bak`. You don't need FTP or a PC.

## Use

The app has two parts: one row per **region** (Europe, Americas, Japan, Asia, other) and, below them, your installed **games**. When a game starts, it gets the language of its region, unless you chose a language for that game.

| Option | Region rows | Game rows |
|---|---|---|
| **Automatic** / **By region** (default) | The regional variant that matches: European games get *Español (España)* and American games get *Español (Latinoamérica)* (same for Portuguese and French) | The language of the game's region |
| **System language** | Your system language | Your system language |
| **A specific language** | That language for every game of the region | That language for this game |

Your system language is always shown in green, marked *(system)*, so you can see when a game will use it.

| Button | Action |
|---|---|
| D-pad up / down | Choose a row (L1 / R1 move a page) |
| D-pad left / right, Cross | Change the language |
| Square | Back to *Automatic* |
| Circle | Exit |

Each change is saved immediately and applies the next time the game starts. When the plugin changes a game's language, the console shows a notification such as `Game Language CUSA12345: es-ES -> es-419`.

## How it works

<p align="center"><img src="docs/how-it-works.svg" alt="How Game Language works" width="90%"></p>

The package contains two parts:

- **The app** (`app/`) lists installed games from their `param.sfo`, installs the plugin and writes your choices to `/data/GoldHEN/game_language.ini`.
- **The plugin** (`plugin/`), loaded by GoldHEN into every game, reads that file when the game starts and answers `sceSystemServiceParamGetInt(LANG)` with the language you chose.

The region comes from the game's Content ID: `EP` = Europe, `UP` = Americas, `JP` = Japan, `HP` = Asia, anything else = other. *Automatic* swaps these pairs to the variant of the game's region: es-ES ⇄ es-419, pt-PT ⇄ pt-BR, fr-FR ⇄ fr-CA. For other languages, and for Japan, Asia and other regions, *Automatic* keeps your system language.

Why by region: a game's files don't list the languages it contains (`param.sfo` only has its name translated), so the plugin can't check which languages a game supports. The region is the most reliable hint.

<details>
<summary>Settings file format</summary>

The app writes this file for you. You can also edit it by hand:

```ini
[default]
europe=auto       ; auto = Automatic, system = System language, or a language code
americas=es-419
japan=en-US
asia=system
other=auto

[CUSA12345]
lang=es-ES        ; this game only (a language code or system)
```

Language codes: `ja en-US fr-FR es-ES de it nl pt-PT ru ko zh-Hant zh-Hans fi sv da no pl pt-BR en-GB tr es-419 ar fr-CA cs hu el ro th vi id`.
</details>

## Limitations

- The game must include the language you choose. Game Language only changes the language the game sees; it can't add translations.
- The region is a hint, not a guarantee: some worldwide releases include both variants, and some games lack the language their region suggests. Choose a language for that game to correct it.
- Some games store the language in their own settings or save file. In those games, use the game's own language menu.
- Game names in Japanese, Chinese, Korean, Arabic or Thai characters can't be displayed by the app's font, so the app shows only their Latin characters or title ID.
- So far it has been checked with unit tests and CI builds only. Reports from real consoles are welcome.

## Build

```sh
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis/PS4Toolchain
export GOLDHEN_SDK=/path/to/GoldHEN_Plugins_SDK   # built with `make`
make          # plugin + app -> app/IV0000-GLNG00001_00-GAMELANGUAGE0000.pkg
make test     # host unit tests for the shared logic
```

Every push builds the package in [GitHub Actions](.github/workflows/build.yml). Pushing a `v*` tag publishes a release with `GameLanguage.pkg` and `game_language.prx`.

## Credits

- [GoldHEN](https://github.com/GoldHEN/GoldHEN) and the [GoldHEN Plugins SDK](https://github.com/GoldHEN/GoldHEN_Plugins_SDK) (MIT)
- [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
- [stb_truetype](https://github.com/nothings/stb) (public domain / MIT)
- [Noto Sans](https://github.com/notofonts/latin-greek-cyrillic) ([OFL](app/assets/fonts/OFL.txt))

License: [GPL-3.0](LICENSE)

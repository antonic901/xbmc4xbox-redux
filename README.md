<p align="center">
  <img src="media/Splash.png" alt="XBMC Logo"/>
</p>

<p align="center">
  <strong>
    <a href="https://discord.com/channels/770816616937160745/1050581194401652736">website</a>
    •
    <a href="https://kodi.wiki/view/Main_Page">docs</a>
    •
    <a href="https://discord.com/channels/770816616937160745/1050581194401652736">community</a>
    •
    <a href="https://github.com/xbmc4xbox/xbmc4xbox.github.io">add-ons</a>
  </strong>
</p>

<p align="center">
  <a href="LICENSE.md"><img alt="License" src="https://img.shields.io/badge/license-GPLv2-blue.svg?style=flat-square"></a>
  <a href="https://github.com/antonic901/xbmc4xbox-redux/pulls"><img alt="PRs Welcome" src="https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square"></a>
  <a href="#how-to-contribute"><img alt="Contributions Welcome" src="https://img.shields.io/badge/contributions-welcome-brightgreen.svg?style=flat-square"></a>
</p>

<h1 align="center">
  Welcome to XBMC Home Theater Software!
</h1>

XBMC4Xbox is a free and open source media player software made solely for the first-generation Xbox video-game console. The software was forked from the XBMC project (now known as Kodi and formerly known as Xbox Media Player) after XBMC removed support for the Xbox console. Other than the audio / video playback and media center functionality, XBMC4Xbox also has the ability to catalog and launch original Xbox games, and homebrew applications such as console emulators from the Xbox's built-in harddrive.

## Suppport us
I'm seeking for Xbox Development Kit (DVT3/4). Consider buying me a coffee so I can buy one!
<a href="https://www.buymeacoffee.com/antonic901" target="_blank"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee" style="height: 41px !important;width: 174px !important;box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;-webkit-box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;" ></a>

## Download links
This software is in alpha and currently we are only providing nightly releases.

**XBMC (Nightly)**: [Download Link](https://github.com/antonic901/xbmc4xbox-redux/releases/download/nightly/XBMC4XBOX.zip)

## Give your media the love it deserves
XBMC is your all-in-one solution for enjoying almost all popular audio, video, and gaming formats. Whether you're streaming multimedia from anywhere in your home or directly from the internet, XBMC handles it effortlessly, supporting virtually any protocol available.

Simply point XBMC to your media or game collection, and watch as it **magically scans and creates a personalized library** complete with box covers, descriptions, and fanart. XBMC isn't just for movies and music; it also offers robust support for video games, including emulators and retro games. Turn your Xbox into the ultimate multimedia and gaming hub, complete with playlist and slideshow features, weather forecasts, audio visualizations, and more. Once installed, your Xbox will become a fully functional multimedia jukebox and gaming console all in one.

<p align="center">
  <img src="https://github.com/xbmc/xbmc/blob/master/docs/resources/kodi.gif" alt="XBMC">
</p>

## Getting Started
XBMC4Xbox is the continuation of the popular [Kodi fork](https://www.xbmc4xbox.org.uk/), specifically developed for the Original Xbox (2001). This project is a revitalized fork of the original XBMC4Xbox, with the primary goal of aligning it with the latest advancements in Kodi software. Our main mission is to keep the GUILIB updated to match Kodi, ensuring that the newest skins can be seamlessly backported. By doing so, we aim to bring the best of modern Kodi features to the classic Xbox platform, preserving its legacy while enhancing its functionality.

## How to Contribute
XBMC is created by users for users and **we welcome every contribution**. There are no highly paid developers or poorly paid support personnel on the phones ready to take your call. There are only users who have seen a problem and done their best to fix it. This means XBMC will always need the contributions of users like you. How can you get involved?

* **Coding:** Developers can help XBMC by **[fixing a bug](https://github.com/antonic901/xbmc4xbox-redux/issues)**, adding new features, making our technology smaller and faster and making development easier for others. XBMC's codebase consists mainly of C++ with small parts written in a variety of coding languages. Our add-ons mainly consist of python and XML.
* **Helping users:** Our support process relies on enthusiastic contributors like you to help others get the most out of XBMC. The #1 priority is always answering questions in our **[support forums](https://discord.com/channels/770816616937160745/1050581194401652736)**. Everyday new people discover XBMC, and everyday they are virtually guaranteed to have questions.
* **Localization:** Translate **XBMC**, **add-ons, skins etc.** into your native language.
* **Add-ons:** **Add-ons** are what make XBMC the most extensible and customizable entertainment hub available. **[Get started building an add-on](https://kodi.tv/create-an-addon)**.

**Not enough free time?** No problem! There are other ways to help XBMC.

* **Spread the word:** Share XBMC with the world! Tell your friends and family about how XBMC creates an amazing entertainment experience.

## Requirements
 - PC or [Virtual Machine](https://www.virtualbox.org/) with Windows XP SP3 or Windows 11
 - Microsoft Visual Studio .NET 2003 with [XDK (Xbox Development Kit)](https://xbox.fandom.com/wiki/Xbox_Development_Kit)
 - [Java 1.8](https://www.openlogic.com/openjdk-downloads)
 - [SWIG](https://www.swig.org/)
 - [Doxygen](https://www.doxygen.nl/)

Unfortunately I can't share any of those software because both Microsoft Visual Studio .NET 2003 and Xbox Development Kit are Microsoft's property.

## Building from source
Before building XBMC, make `java` is properly installed and accessible within CMD. To do that open CMD and run:

```
java -version
openjdk version "1.8.0_452"
OpenJDK Runtime Environment (build 1.8.0_452-b09)
OpenJDK 64-Bit Server VM (build 25.452-b09, mixed mode)
```

After that, download `doxygen` and `swig` tools from this [link](https://www.dropbox.com/scl/fi/5ymtc9cqnjjpx7gw8i6zz/BuildDependencies.zip?rlkey=o2rygnv6lrv18zm01v8q5azq3&st=k51dwl2y&dl=1). Unzip archive and put `BuildDependencies` inside `tools` folder.

First step is to generate Python binding code. You can do that by navigating to `tools->codegenerator` and running generator script:
```bash
.\GenerateWIN.bat
```

Second step is to generate dummy `svn_rev.h` file. To do that simply call script:
```bash
.\update_svn_rev.bat
```

Finally run build script:
```bash
.\Build.bat noprompt build1
```

If everything went well, inside BUILD folder you will find XBMC ready to be installed on Xbox. Rename BUILD folder to XBMC and use FTP to transfer files to Xbox.

## Acknowledgements
XBMC couldn't exist without

* All the **[contributors](https://github.com/antonic901/xbmc4xbox-redux/graphs/contributors)**. Big or small a change, it does make a difference.
* All the developers that write the fantastic **software and libraries** that XBMC uses. We stand on the shoulders of giants.
* Our **fantastic community** at **[XBOX-SCENE](https://discord.gg/VcdSfajQGK)** for all the support!

## License
XBMC is **GPLv2 licensed**. You may use, distribute and copy it under the license terms.

<a href="https://github.com/antonic901/xbmc4xbox-redux/graphs/contributors"><img src="https://forthebadge.com/images/badges/built-by-developers.svg" height="25"></a>
<a href="https://github.com/antonic901/xbmc4xbox-redux"><img src="https://forthebadge.com/images/badges/certified-cousin-terio.svg" height="25"></a>
<a href="https://github.com/antonic901/xbmc4xbox-redux"><img src="https://forthebadge.com/images/badges/approved-by-george-costanza.svg" height="25"></a>
<a href="https://github.com/antonic901/xbmc4xbox-redux/releases"><img src="https://forthebadge.com/images/badges/check-it-out.svg" height="25"></a>
<a href="https://github.com/antonic901/xbmc4xbox-redux"><img src="https://forthebadge.com/images/badges/winter-is-coming.svg" height="25"></a>

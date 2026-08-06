# OpenKJ 3.0 — Community Maintenance Release

Hey OpenKJ Community,

Like many of you, I've relied on OpenKJ to run shows for years. But as we all know, the upstream project has been largely abandoned for over two years—leaving us with buggy builds, outdated dependencies, and a dead hosting backend.

Rather than letting it rot, I patched up the codebase to give the community a working, stable build again.

## 🛠️ 1. Free OpenKJ Bug-Fix & Maintenance Release

I've updated OpenKJ with modern build fixes and performance optimizations. Native builds are available for Windows, Linux, and macOS.

- **Cost:** 100% free and open source
- **Download:** [GitHub Releases](https://github.com/pkircher29/OpenKJ/releases)
- **What changed:** see [RELEASE_NOTES.md](RELEASE_NOTES.md)

## 🌐 2. OpenKJ Web Hosting is Back ($5/month)

If you loved OpenKJ's web-hosting features for online songbooks and requests, you probably noticed the old backend went dark. I am now hosting dedicated OpenKJ web servers so you can get your online request system back online.

- **Old price:** $10/month
- **My price:** $5/month — locked in forever for OpenKJ users

## 🚀 3. The "Not-So-Secret" Upgrade: Auto-KJ (+ 60-Day Free Trial)

Maintaining OpenKJ made me realize that fixing a decade-old codebase could only go so far. To solve its limitations permanently, I built a brand-new platform called [Auto-KJ](https://auto-kj.com).

I want to make it as easy as possible for you to check it out:

- **60-day free trial:** Everyone gets two full months of Auto-KJ completely free.
- **One-click history import:** Don't lose your rotation data. You can easily import your existing OpenKJ singer histories and song databases straight into Auto-KJ.

What happens after your 60-day trial? You have total control:

- **Stick with Auto-KJ:** Keep your premium subscription for the full modern feature set and automated web request engine.
- **Revert to hosted OpenKJ:** Downgrade back to OpenKJ with web hosting for just $5/month forever.
- **Go 100% free with Auto-KJ Offline:** If you don't need a web server and prefer running old-school shows with paper slips and pencils, you can use the free tier of Auto-KJ indefinitely. No web features, no subscription, no cost.

## How to Get Started

1. **Grab the OpenKJ bug-fix update:** [GitHub Releases](https://github.com/pkircher29/OpenKJ/releases)
2. **Claim your 60-day free Auto-KJ trial and import your history:** [auto-kj.com](https://auto-kj.com/#pricing)
3. **Sign up for $5/month OpenKJ hosting:** [auto-kj.com](https://auto-kj.com/#pricing)

No traps, no forced upgrades—just better software for KJs.

---

## About OpenKJ

Cross-platform open source karaoke show hosting software.

OpenKJ is a fully featured karaoke hosting program.
A few features:
* Save/track/load regular singers
* Key changer
* Tempo control
* EQ
* End of track silence detection (after last CDG draw command)
* Rotation ticker on the CDG display
* Option to use a custom background or display a rotating slide show on the CDG output dialog while idle
* Fades break music in and out automatically when karaoke tracks start/end
* Remote request server integration allowing singers to look up and submit songs via the web or mobile apps
* Automatic performance recording
* Autoplay karaoke mode
* Lots of other little things

It currently handles media+g zip files (zip files containing an mp3, wav, or ogg file and a cdg file) and paired mp3 and cdg files. It also can play non-cdg based video files (mkv, mp4, mpg, avi) for both break music and karaoke.

Database entries for the songs are based on the file naming scheme. The common patterns are included, which should cover 90% of what's out there. Custom patterns can also be defined in the program using regular expressions.

## Building OpenKJ

**Requirements:**

* Qt 5.x
* gstreamer 1.4 or above
* spdlog
* taglib

**Linux**

Build using cmake from the command line or in your IDE of choice.

**Mac**

Building works on macOS in Qt Creator using the native xcode compiler. Use the latest stable version of the GStreamer SDK from http://gstreamer.freedesktop.org.

**Windows**

Building works on Windows in Qt Creator using the msvc build system (both 32 and 64 bit). Use the latest stable version of the GStreamer SDK from http://gstreamer.freedesktop.org. Prebuilt installers are available on the [releases page](https://github.com/pkircher29/OpenKJ/releases) if you just want to run the software rather than build it yourself.

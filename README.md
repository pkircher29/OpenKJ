# OpenKJ 3.0 — Community Maintenance Release

Hey OpenKJ Community,

Like many of you, I've relied on OpenKJ to run shows for years. Development upstream has slowed to a crawl, and the builds most of us are running carry bugs that have been there a long time.

Rather than let it sit, I went through the codebase properly — not a quick patch, a full reliability and security audit across playback, the database layer, networking, and the build system.

## 🛠️ 1. Free OpenKJ Bug-Fix & Maintenance Release

Native builds for Windows, Linux, and macOS. Everything below is fixed in 3.0:

**Crashes and memory safety**
- Two confirmed GStreamer double-unrefs — one on media teardown, one in audio recording
- Undefined behavior in CD+G scrolling, plus the oversized stack buffers behind it
- A use-after-free in background directory scanning
- Uninitialized video image formats; malformed frames now rejected safely

**Data loss and corruption**
- Singer history refresh was *clearing* valid history instead of loading it
- Songbook uploads silently dropped rows at every 1,000-song boundary
- Recordings now wait for EOS so encoded files finalize cleanly

**Security**
- User-controlled SQL converted to prepared statements with bound parameters
- Legacy card/CVV persistence disabled and purged

**Performance and stability**
- Songbook PDF generation: N+1 database access replaced with one ordered query
- SongShop login failures and malformed replies no longer trap the UI in an endless event loop

That is the short list. The full audit — every fix, with the reasoning — is in [RELEASE_NOTES.md](RELEASE_NOTES.md).

- **Cost:** 100% free and open source
- **Download:** [GitHub Releases](https://github.com/pkircher29/OpenKJ/releases)

## 🌐 2. OpenKJ Web Hosting is Back ($5/month)

If you pay for OpenKJ web hosting for your online songbook and requests, I'm now running compatible servers at half the price. Point OpenKJ at my server, paste your API key, and everything works the same — same protocol, same workflow. Switching back is just as easy, so there's nothing to lose by trying it.

- **Their price:** from $9.99/month
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

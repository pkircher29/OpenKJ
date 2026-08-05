# OpenKJ 3.0

**A free, community-maintained release of cross-platform open-source karaoke show-hosting software.**

OpenKJ 3.0 is a reliability, security, and performance maintenance release for KJs who want to keep using OpenKJ. It is not an official replacement for upstream OpenKJ support or hosting; it is a community-maintained alternative with an independently audited codebase and native Windows, Linux, and macOS build targets.

## OpenKJ 3.0 community release

- **Free and open source:** the 3.0 bug-fix and maintenance release remains available at no cost.
- **What changed:** see [RELEASE_NOTES.md](RELEASE_NOTES.md) for the verified reliability, playback, database, Songbook, and legacy-payment-setting fixes.
- **Announcement and choices:** see [COMMUNITY_ANNOUNCEMENT.md](COMMUNITY_ANNOUNCEMENT.md) for the full community note, including the optional OpenKJ-compatible hosted-service offering and Auto-KJ migration/trial paths.
- **Downloads:** the verified GitHub Release link will be added here when publication completes. Do not treat an unverified build artifact as a production installer.

OpenKJ remains a useful option for hosts who prefer its workflow. If you want a newer platform, [Auto-KJ](https://auto-kj.com/#pricing) offers a separate optional migration path, including importing OpenKJ singer histories and song databases. There are no forced upgrades.

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

It currently handles media+g zip files (zip files containing an mp3, wav, or ogg file and a cdg file) and paired mp3 and cdg files.  I'll be adding others in the future if anyone expresses interest.  It also can play non-cdg based video files (mkv, mp4, mpg, avi) for both break music and karaoke.

Database entries for the songs are based on the file naming scheme.  I've included the common ones I've come across which should cover 90% of what's out there. Custom patterns can be also defined in the program using regular expressions.



**Requirements to build OpenKJ:**

* Qt 5.x
* gstreamer 1.4 or above
* spdlog
* taglib

**Linux**

Build using cmake from the command line or in your IDE of choice

**Mac**

Building now works on OS X in Qt Creator using the native xcode compiler.  Use the latest stable version of the GStreamer SDK from http://gstreamer.freedesktop.org.


**Windows**

Building now works on Windows in Qt Creator using the msvc build system (both 32 and 64 bit).  Use the latest stable version of the GStreamer SDK from http://gstreamer.freedesktop.org.  You will likely need to modify the paths in the OpenKJ.pro file to match your devel environment.  Installers can be found at http://openkj.org/ if you just want to run the software and not build it yourself or help out with development.


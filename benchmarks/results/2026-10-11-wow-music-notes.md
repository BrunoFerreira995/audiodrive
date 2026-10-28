# WoW and music investigation — 2026-10-11

Observed World of Warcraft running at approximately 239.8% CPU in a process snapshot. Music.app was present; its presence does not confirm active playback. Default output: BuiltInHeadphoneOutputDevice (external headphones).

Native CoreAudio playback probe: 30 seconds, 48 kHz stereo, requested period 128 frames, no synthetic CPU load. See `2026-10-11-wow-music.csv`. Recorded 11,252 callbacks, zero sample-buffer starvations, zero callback processing budget overruns, backend error 0. Maximum processing time: 1,447.25 microseconds. These counters do not measure device-level underruns or scheduler lateness.

Recent unified CoreAudio logs contained two overload events at 19:21:34.519 and 19:21:34.554 (timestamps as emitted by log show):

> HALS_OverloadMessage: Overload possibly due to client timeout.

Both audio analytics events identified `org.mozilla.firefox` as HostApplicationDisplayID, Headphones as output source, and `com.blizzard.worldofwarcraft`, Firefox, and CoreSpeech among active clients. The records showed scheduler_latency values of 11791 and 10041 (units not verified), and a smallest buffer frame size of 256. Client sample rates were approximately 44.1 kHz; WoW's listed I/O buffer size was 1024 frames, Firefox's 256.

Interpretation: system audio overload was observed while WoW was active. These events support investigating client scheduling delays; they do not establish that Firefox caused the overload, that the music was playing through Firefox, or that Audio32 has a data race. No application or device settings were changed.

## Mitigation attempt

The user subsequently confirmed interference only with simultaneous playback and requested a fix. Additional CoreAudio overload records still identified Firefox with a 256-frame client buffer while WoW's listed buffer was 1024 frames.

The default output device reported a settable buffer of 512 frames (range 14–4096). A live change to 1024 frames initially succeeded but later read back as 512; it was not a durable mitigation. No polling or forced buffer override was installed.

macOS denied Firefox profile access even with command escalation. Using Mozilla's documented per-user macOS policy interface instead, two audio preferences were written to the `org.mozilla.firefox` defaults domain:

- `media.cubeb_latency_playback_ms`: 100, numeric, Status=user.
- `media.cubeb_latency_mtg_frames`: 2048, numeric, Status=user.

`EnterprisePoliciesEnabled` was enabled for this per-user configuration. This may cause Firefox to display its policies/management indication. Existing macOS Firefox defaults were exported to `/tmp/audio32-firefox-before.plist` and preserved in the imported configuration. Both preference names and the macOS configuration interface were verified against Mozilla's source and documentation:

- https://searchfox.org/firefox-main/source/dom/media/CubebUtils.cpp
- https://firefox-admin-docs.mozilla.org/reference/policies/preferences/
- https://firefox-admin-docs.mozilla.org/reference/policies/enterprisepoliciesenabled/

Firefox was quit gracefully via AppleScript and reopened after exit, at approximately 19:34:23 local time. WoW remained running. The preferences were confirmed in the macOS defaults domain; effective Firefox values and successful simultaneous playback still require verification. This is a buffering mitigation attempt, not a confirmed root-cause fix.

Rollback before discarding the temporary backup: restore the two policy keys (`Preferences` and `EnterprisePoliciesEnabled`) from the exported plist, preserving any other newer macOS defaults. Because Status=user sets Firefox profile values on startup, reset these two audio values in about:config as well; the original profile values could not be read. Restart Firefox after rollback.

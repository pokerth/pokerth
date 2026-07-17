# Third-party web services used by the client

This document lists the external web services the PokerTH client may contact at
runtime, so their use stays transparent and legally clean. All requests
originate from the end user's own IP address; PokerTH operates no proxy for
them.

## Chat translation

Feature: an optional globe symbol (🌐) shown next to incoming chat messages
(lobby and in-game). Tapping it translates that single message into the
language the user has selected in the client.

Implementation: `src/gui/qt6-qml/cpp/chattranslator.cpp`
Setting: `AllowChatTranslation` (default on). Nothing is ever transmitted until
the user explicitly taps the symbol of a specific message — there is no
automatic or bulk translation.

### Primary: Google Translate ("gtx" endpoint)

- Endpoint: `https://translate.googleapis.com/translate_a/single`
  (`client=gtx`, `sl=auto`, `tl=<client language>`, `dt=t`, `q=<message>`)
- No API key. Free, keyless endpoint; the request is made directly from the
  user's client. Source language is auto-detected.
- This is the same public endpoint used by many open-source tools
  (e.g. translate-shell). It is not a contractually guaranteed API and Google
  may change or rate-limit it; the client therefore degrades gracefully (see
  fallback) and never blocks the UI.
- Terms: https://policies.google.com/terms — the translated text is the chat
  message the user chose to translate. No account data or credentials are sent.

### Fallback: MyMemory

- Endpoint: `https://api.mymemory.translated.net/get`
  (`q=<message>`, `langpair=<source>|<client language>`)
- Officially free, keyless API. Used only when the primary endpoint fails.
  MyMemory requires an explicit source language; since auto-detection is then
  unavailable, English is assumed as a heuristic (the most common foreign
  language in the international lobby). If the client's own language is English
  the fallback is skipped and the user can simply retry.
- Anonymous usage limit is per IP (~5000 words/day); this is ample for
  interactive, on-demand chat translation.
- Terms: https://mymemory.translated.net/doc/usagelimits.php

### Privacy note

The text sent to the translation service is exactly the chat message the user
chose to translate — nothing more. Users who do not want any chat text to leave
their machine can disable the feature entirely via the
"Chat-Übersetzung anbieten" setting (`AllowChatTranslation`).

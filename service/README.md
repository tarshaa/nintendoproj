# Automatic DSi photo-game test

This is a new network build, separate from the previous offline app. Intended loop:

1. DSi captures and saves the original photo.
2. DSi connects to a saved Wi-Fi network in DSi mode.
3. An authenticated HTTPS echo tests the encrypted connection using synthetic bytes.
4. The console uploads the captured `.pwg` file to the paired service.
5. The service sends that image and the canonical project guidance to a vision model.
6. It validates the returned scene-linked platformer data, then runs the native movement code to verify alternate routes, recovery and completion reachability.
7. The console downloads the small package, checks its integrity and photo linkage, saves it next to the photo, and launches it. Saved games replay offline.

This implementation is an initial **platformer pipeline test**, not a finished universal game generator. Runtime silhouettes currently cover mushrooms, leaves, books, boxes, fruit, plates and cloth. Unsupported photos and invalid routes fail explicitly; they do not secretly load a fixed substitute game. Photo-specific objects, positions, heights and spring behavior are generated, within a preinstalled movement engine. Generated art uses the existing bitmap display and a following projection; it is not the final browser-quality 3D port.

## Private API setup

Create an API account/key at https://platform.openai.com/api-keys and enable API billing if required. Run `python3 service/setup_key.py` in a real terminal, or open the workspace's `Set up Photo Worlds API.command`. Input is hidden. This only saves credentials, with owner-only permissions, under the ignored `service/.private/` directory. It makes no paid request. Do not paste the key into chat or put it on the SD card.

Default model is `gpt-4.1-mini`; override with `PHOTO_WORLDS_MODEL`. The service allows ten new test jobs per UTC day, with at most two vision attempts per job. This is a request cap, not a dollar cap. Real use requires available API credit. Logs do not print credentials or photo bodies.

The canonical rules are loaded from the parent workspace's `outputs/adventure-games/PROJECT-RULES.md`; keep this workspace layout when running the service. No competing design-rules copy is maintained here.

## Service and one-time pairing

`python3 service/server.py` binds `127.0.0.1:8788`. For direct LAN connections, provide `--host`, `--cert` and `--key`; plaintext LAN binding is refused. Alternatively place only this service behind an HTTPS tunnel. Do not expose the browser game directory or the private data directory.

The SD card needs `photo-worlds/network.cfg`: first line the exact HTTPS base URL, second line a dedicated 64-character device token from private settings. This is a service pairing token, **not** the model API key. `photo-worlds/service-ca.pem` contains the CA certificate(s) that authenticate that service. The client requires peer and hostname validation; the DSi clock must be correct. No insecure certificate bypass is implemented. Pairing artifacts must be excluded from source control.

For an iPhone, enable Allow Others to Join and Maximize Compatibility. Configure its hotspot in the DSi's advanced network settings. The WPA2 password remains on the console. Real iPhone association, DHCP, certificate verification and photo round trip still require hardware testing.

## Build and install

The separate GitHub Actions `network-build` job installs BlocksDS libcurl/Mbed TLS/zlib dependencies and builds with `make PHOTO_NETWORK=1`. The output is `photo-worlds-online.nds`. Install alongside the existing app; do not replace BOOT.NDS or change startup before the test passes. Launch the network app in DSi mode. Browser JavaScript games do not run on the DSi.

The online app offers A to generate from the photo review, B to cancel, and A to retry after network failure. Saved photos without a generated package can be retried from the calendar. A failed generation never opens the palette-only fallback in this build. Save/package data is committed via temporary files.

## Evidence required

Run local checks with `python3 -m unittest discover -s service -p test_service.py -v`.

These tests inject synthetic vision output and never call the paid API. They cover authentication, exact echo, upload/polling, corrupt captures, invalid/overlapping/unreachable geometry, duplicate job reuse, explicit provider failure, binary validation and native movement/recovery. They **do not prove** live photo recognition, handheld Wi-Fi/TLS, frame rate, visual quality, user comprehension or under-three-minute latency.

Before claiming the automatic loop works, record at least two different real DSi captures; the identified objects and game geometry must differ meaningfully. Record capture-to-first-playable-frame, stage timings, a complete playable route, a failed/cancelled request, and offline replay after power cycling. Keep the sources and provenance local and private.

Sources: [BlocksDS Wi-Fi/TLS](https://blocksds.skylyrac.net/tutorial/advanced/wifi/), [DSi Internet mode](https://blocksds.skylyrac.net/dswifi/md_documentation_2internet__mode.html), [OpenAI image inputs](https://developers.openai.com/api/docs/guides/images-vision), [structured output](https://developers.openai.com/api/docs/guides/structured-outputs?api-mode=responses).

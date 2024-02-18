#!/bin/bash

rm -f "~/Library/Application Support/AU Lab/com.apple.audio.aulab_componentcache.plist"
rm -rf "~/Library/Caches/AudioUnitCache"
killall -9 AudioComponentRegistrar
sudo killall -9 AudioComponentRegistrar
exit 0

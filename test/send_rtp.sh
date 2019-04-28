#!/bin/sh
gst-launch-1.0 pulsesrc ! audioconvert ! audio/x-raw,channels=1,depth=16,width=16,rate=8000 ! rtpL16pay  ! udpsink host=192.168.1.61 port=40000
#gst-launch-1.0 osxaudiosrc ! audio/x-raw,channels=1,depth=16,width=16,rate=8000 ! rtpL16pay  ! udpsink host=192.168.1.164 port=40000
#gst-launch-1.0 -v alsasrc name=mic provide-clock=true do-timestamp=true buffer-time=100000 mic. ! audio/x-raw, format=S16LE, channels=1, width=16,depth=16,rate=8000 ! audioconvert ! rtpL16pay ! queue ! udpsink host=192.168.1.164 port=40000 sync=false

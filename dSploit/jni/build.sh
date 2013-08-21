#!/bin/sh
ndk-build clean && \
ndk-build && \
unzip ../assets/tools.zip && \
rm ./tools/arpspoof/arpspoof && \
mv ../libs/armeabi/arpspoof ./tools/arpspoof/ && \
rm ./tools/tcpdump/tcpdump && \
mv ../libs/armeabi/tcpdump ./tools/tcpdump/ && \
zip -r tools.zip tools && \
rm -rf tools && \
mv tools.zip ../assets/
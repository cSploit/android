#!/bin/sh
rm -rf ../libs/armeabi/arpspoof && \
ndk-build clean && \
ndk-build && \
unzip ../assets/tools.zip && \
rm ./tools/arpspoof/arpspoof && \
mv ../libs/armeabi/arpspoof ./tools/arpspoof/ && \
zip -r tools.zip tools && \
rm -rf tools && \
mv tools.zip ../assets/
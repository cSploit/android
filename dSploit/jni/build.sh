#!/bin/sh
ndk-build -j$(grep -E "^processor" /proc/cpuinfo | wc -l) && \
unzip ../assets/tools.zip && \
for tool in arpspoof tcpdump ettercap hydra nmap; do \
	mv ../libs/armeabi/$tool ./tools/$tool/$tool; \
done && \
zip -r tools.zip tools && \
rm -rf tools && \
mv tools.zip ../assets/

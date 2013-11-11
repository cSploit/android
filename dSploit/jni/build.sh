#!/bin/sh
ndk-build -j$(grep -E "^processor" /proc/cpuinfo | wc -l) && \
unzip -q ../assets/tools.zip && \
for tool in arpspoof tcpdump ettercap hydra nmap; do \
	mv ../libs/armeabi/$tool ./tools/$tool/$tool; \
done && \
find ./nmap -name "*.lua" -print0 | rsync -aq --files-from=- --from0 ./ ./tools/ && \
rsync -aq ./nmap/scripts/ ./tools/nmap/scripts/ &&
zip -qr tools.zip tools && \
rm -rf tools && \
mv tools.zip ../assets/

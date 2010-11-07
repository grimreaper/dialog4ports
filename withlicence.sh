make
testingPort="/usr/ports/x11-wm/icewm"

WITH_XORG=WITH_XORG	./dialog4ports	$(make -C $testingPort -V PORTNAME)="$(make -C $testingPort -V COMMENT)"  \
				--licence BSDL \
				--licence-text lic.txt \
				--option WITH_XORG='use xorg' \
				--option WITHOUT_FOO='do not use foo'=info.txt \
				--radio WWW_TYPE='use this for a web server!'=apache#lighttpd#god#heaven \
				--input LANGS='use a language '=n.txt \
				--option XYZ='argi'		\
				--option AAAAZ='hello my dear'

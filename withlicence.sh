testingPort="/usr/ports/x11-wm/icewm"

WITH_XORG=WITH_XORG	./dialog4ports	$(make -C $testingPort -V PORTNAME)="$(make -C $testingPort -V COMMENT)"  \
					--showLicence \
				WITH_XORG='use xorg'=% \
				WITHOUT_FOO='do not use foo'=%=info.txt \
				WWW_TYPE='use this for a web server!'=apache#lighttpd#god#heaven \
				LANGS='use a language '=-=n.txt \
				XYZ='argi'=-		\
				AAAAZ='hello my dear'=%

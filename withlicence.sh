make
testingPort="/usr/ports/x11-wm/icewm"

WITH_XORG=WITH_XORG	./dialog4ports \
				--port	icewm \
				--port-comment	"an evil thing" \
				--licence	BSDL \
				--option	WITH_XORG='use xorg' \
				--option	WITHOUT_FOO='do not use foo' --hfile info.txt \
				--radio	WWW_TYPE='use this for a web server!'=apache#lighttpd#god#heaven \
				--input	LANGS='use a language ' --hfilen.txt \
				--option	XYZ='argi'		\
				--option	AAAAZ='hello my dear' \

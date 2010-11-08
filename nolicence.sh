make
testingPort="/usr/ports/x11-wm/icewm"

WITH_XORG=WITH_XORG	./dialog4ports \
				--port	icewm \
				--port-comment	"an evil thing" \
				--option	WITH_XORG='use xorg' \
				--option	WITHOUT_FOO='do not use foo' --hfile info.txt \
				--radio	WWW_TYPE='use this for a web server!'=apache#lighttpd#god#heaven \
				--radio	GR_TYPE='use this for a web server!'=X11#CONSOLE#evil --required \
				--input	LANGS='use a language' --hfile n.txt \
				--option	XYZ='argi'		\
				--option	AAAAZ='hello my dear'

WITH_XORG=WITH_XORG	./dialog4ports --portname name \
				--check WITH_XORG='use xorg for this' \
				--check WITHOUT_HELLO='dont use that' \
				--radio APACHE='fineprint' \
					LIGHTTPD='or not' \
					PYTHON='use python to host me' \
				--input LANG='choose a language' \
				--check life='use life....'

				

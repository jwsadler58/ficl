SOURCES = softcore.fr jhlocal.fr marker.fr prefix.fr ifbrack.fr oo.fr classes.fr string.fr fileaccess.fr

softcore.c: softcore.py  $(SOURCES)
	python3 softcore.py $(SOURCES) >./softcore.c

clean: 
	rm *.c


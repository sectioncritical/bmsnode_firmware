
MDFILES=mainpage.md license.md ../packet.md ../command.md ../blink-patterns.md

all: docs

docs: html/index.html

html/index.html: $(MDFILES)
	rm -rf html
	doxygen

clean:
	rm -rf html


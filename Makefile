# mruby is using Rake (http://rake.rubyforge.org) as a build tool.
# We provide a minimalistic version called minirake inside of our
# codebase.

RAKE = ruby ./minirake
PREFIX ?= /usr/local

all :
	$(RAKE)
.PHONY : all

test : all
	$(RAKE) test
.PHONY : test

clean :
	$(RAKE) clean
.PHONY : clean

install : all
	install -m 4755 bin/makeseed bin/makezft $(PREFIX)/bin
.PHONY : install

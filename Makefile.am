EXTRA_DIST = Copyright examples solaris tests
WIN32_DIST = README TODO Copyright COPYING AUTHORS ChangeLog INSTALL NEWS xml.exe
WIN32_doc_DIST = $(generated_docs) doc/html.css

ACLOCAL_AMFLAGS = -I m4
if GCC
AM_CFLAGS = -Wall -pedantic
endif

GIT_DESCRIBE = git describe --tags --dirty

# TODO: figure out why log removal is needed and/or make it completely automatic
dist-hook:
	@for log in $(srcdir)/examples/*.log ; do if [ -f "$$log" ] ; then \
	  rm $(srcdir)/examples/*.log ; \
	  echo 'ERROR: log files need to cleaned BEFORE making tarball, try again'; exit 1 ; fi ; done
	@ if [ -d .git ] ; then \
	    if [ $(VERSION) != `$(GIT_DESCRIBE)` ] ; then \
		echo 'ERROR: version mismatch, rerun autoconf -f' ; exit 1 ; fi ; \
	    $(SED) 's/\[m4_esyscmd_s(\[$(GIT_DESCRIBE)\])\]/[$(VERSION)]/' \
	      $(srcdir)/configure.ac > $(distdir)/configure.ac ; fi

cleantar:
	@(rm -f xmlstarlet*.tar.gz)

rpm: cleantar distdir xmlstarlet.spec
	cp xmlstarlet.spec $(distdir)
	$(AMTAR) -czf $(distdir).tar.gz $(distdir)
	rpmbuild -ta $(distdir).tar.gz

$(distdir)-win32.zip: $(WIN32_DIST) $(WIN32_doc_DIST)
	mkdir -p /tmp/$(distdir)/doc
	cp $^ /tmp/$(distdir)
	cd /tmp/$(distdir) && mv $(notdir $(WIN32_doc_DIST)) /tmp/$(distdir)/doc
	strip /tmp/$(distdir)/xml.exe
	cd /tmp && zip -9 --symlinks --recurse-paths --to-crlf --move \
	  $(distdir)-win32.zip $(distdir)
	mv /tmp/$(distdir)-win32.zip .

dist-win32: $(distdir)-win32.zip

version.h: VERSION
	@if [ -d .git ] ; then \
	    VERSION=`$(GIT_DESCRIBE)`; \
	else VERSION=$(VERSION) ; fi; \
	NEW_VERSION="#define VERSION \"$$VERSION\""; \
	OLD_VERSION=`cat version.h 2>/dev/null` ; \
	if [ "$$NEW_VERSION" != "$$OLD_VERSION" ] ; then \
	    echo "$$NEW_VERSION" > version.h ; \
	    echo "version.h: $$NEW_VERSION"; fi
VERSION:

# need to build version.h even if dependency files haven't been
# generated
src/xml.o : version.h



# testing
include examples/tests.mk

# building executable
bin_PROGRAMS = xml

include src/sources.mk
xml_SOURCES += version.h
nodist_xml_SOURCES = $(generated_usage_sources)
EXTRA_DIST += $(usage_texts) usage2c.awk

.txt.c:
	$(AM_V_GEN)$(AWK) -f $(srcdir)/usage2c.awk $< > $@
$(generated_usage_sources) : usage2c.awk


# doc
include doc/doc.mk

man_MANS = $(manpage)
dist_doc_DATA = $(userguide_gen) doc/html.css $(txtguide)
EXTRA_DIST += $(manpage_src) $(userguide_src) $(txtguide_src)
EXTRA_DIST += $(buildfiles_docs) $(manpage)


CLEANFILES = version.h $(generated_usage_sources)
MAINTAINERCLEANFILES = $(manpage) $(userguide_gen) $(txtguide)

.PHONY: cleantar rpm dist-win32 VERSION doc

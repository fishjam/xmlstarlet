userguide = doc/xmlstarlet-ug
userguide_gen = $(userguide).html
if HAVE_FOP
userguide_gen += $(userguide).pdf
if HAVE_PDF2PS
userguide_gen += $(userguide).ps
endif
endif
userguide_src = $(srcdir)/$(userguide).xml

txtguide = doc/xmlstarlet.txt
txtguide_src = $(srcdir)/doc/gen-doc

manpage = doc/xmlstarlet.1
manpage_src = $(srcdir)/doc/xmlstarlet-man.xml

generated_docs = $(userguide_gen) $(txtguide) $(manpage)
buildfiles_docs = $(srcdir)/doc/replace-PROG-VERSION.xsl $(srcdir)/doc/xmlstar-fodoc-style.xsl

DOCBOOK_PARAMS = \
--param section.autolabel 1 \
--stringparam generate.toc 'book toc,title'

EDIT_XML = $(XSLTPROC) \
  --stringparam VERSION '$(VERSION)' \
  --stringparam PROG "`echo xml | $(SED) '$(program_transform_name)'`" \
  $(srcdir)/doc/replace-PROG-VERSION.xsl

if BUILD_DOCS

.xml.html:
	$(V_DOCBOOK)$(EDIT_XML) $< | $(XSLTPROC) $(DOCBOOK_PARAMS) \
  --stringparam html.stylesheet html.css \
  http://docbook.sourceforge.net/release/xsl-ns/current/html/docbook.xsl \
  - > $@

.xml.fo:
	$(V_DOCBOOK)$(EDIT_XML) $< | $(XSLTPROC) $(DOCBOOK_PARAMS) $(srcdir)/doc/xmlstar-fodoc-style.xsl - > $@

if HAVE_FOP
.fo.pdf:
	$(V_FOP)$(FOP) -q $< $@
endif
if HAVE_PDF2PS
.pdf.ps:
	$(AM_V_GEN)$(PDF2PS) $< $@
endif

$(userguide).html : $(userguide_src)

$(manpage): $(manpage_src)
	 $(V_DOCBOOK)$(EDIT_XML) $< | $(XSLTPROC) -o $@ \
	  http://docbook.sourceforge.net/release/xsl-ns/current/manpages/docbook.xsl \
	  -

$(txtguide): $(txtguide_src) $(usage_texts)
	$(AM_V_GEN)srcdir=$(srcdir) SED=$(SED) transform='$(program_transform_name)' $< ./xml > $@

clean-doc:
	rm -f $(generated_docs)

endif BUILD_DOCS

# NOTE: if put inside "if BUILD_DOCS" automake issues a warning
.PHONY: clean-doc

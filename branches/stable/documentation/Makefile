#
# Makefile for mondorescue documentation build
#
# $Id$
#
TARGET=mondorescue-howto
SRC=$(shell ls *.sgml)
DSL=$(TARGET).dsl
IMAGESDIR=images
IMAGES=$(shell ls ${IMAGESDIR}/*.png | sed 's/\.png/.eps/g')

#
# Man pages handling
#
MRMPDIR=../mondo/mondo/mondorestore
MRMPNAM=mondorestore.8
MAMPDIR=../mondo/mondo/mondoarchive
MAMPNAM=mondoarchive.8
MIMPDIR=../mindi
MIMPNAM=mindi.8

all: all-howto all-man

all-howto: $(TARGET).ps $(TARGET).pdf $(TARGET)/index.html $(TARGET).txt $(TARGET).html $(TARGET).rtf

all-man: .all-man-html .all-man-pdf .all-man-txt

.all-man-html: $(MRMPNAM).html $(MAMPNAM).html $(MIMPNAM).html
	@echo ""
	@echo "Generating Man pages in HTML format"
	@echo "-----------------------------------"
	@touch .all-man-html

.all-man-pdf: $(MRMPNAM).pdf $(MAMPNAM).pdf $(MIMPNAM).pdf
	@echo ""
	@echo "Generating Man pages in PDF and PS format"
	@echo "-----------------------------------------"
	@touch .all-man-pdf

.all-man-txt: $(MRMPNAM).txt $(MAMPNAM).txt $(MIMPNAM).txt
	@echo ""
	@echo "Generating Man pages in TXT format"
	@echo "----------------------------------"
	@touch .all-man-txt

$(MRMPNAM).html: $(MRMPDIR)/$(MRMPNAM)
	@man2html -r $< > $@

$(MAMPNAM).html: $(MAMPDIR)/$(MAMPNAM)
	@man2html -r $< > $@

$(MIMPNAM).html: $(MIMPDIR)/$(MIMPNAM)
	@man2html -r $< > $@

$(MRMPNAM).ps: $(MRMPDIR)/$(MRMPNAM)
	@man2dvi $< > $(MRMPNAM).dvi
	@dvips $(MRMPNAM).dvi -o $@
	@rm -f $(MRMPNAM).dvi

$(MAMPNAM).ps: $(MAMPDIR)/$(MAMPNAM)
	@man2dvi $< > $(MAMPNAM).dvi
	@dvips $(MAMPNAM).dvi -o $@
	@rm -f $(MAMPNAM).dvi

$(MIMPNAM).ps: $(MIMPDIR)/$(MIMPNAM)
	@man2dvi $< > $(MIMPNAM).dvi
	@dvips $(MIMPNAM).dvi -o $@
	@rm -f $(MIMPNAM).dvi

$(MRMPNAM).pdf: $(MRMPNAM).ps
	@ps2pdf $<

$(MAMPNAM).pdf: $(MAMPNAM).ps
	@ps2pdf $<

$(MIMPNAM).pdf: $(MIMPNAM).ps
	@ps2pdf $<

$(MRMPNAM).txt: $(MRMPDIR)/$(MRMPNAM)
	@nroff -man $< > $@

$(MAMPNAM).txt: $(MAMPDIR)/$(MAMPNAM)
	@nroff -man $< > $@

$(MIMPNAM).txt: $(MIMPDIR)/$(MIMPNAM)
	@nroff -man $< > $@

${IMAGESDIR}/%.eps: ${IMAGESDIR}/%.png
	@echo "Converting $< to $@"
	@convert $< $@
		
$(TARGET).txt: $(SRC)
	@echo ""
	@echo "Generating doc in TXT format"
	@echo "----------------------------"
	@docbook2txt $(TARGET).sgml

$(TARGET).ps: $(SRC) $(DSL) $(IMAGES)
	@echo ""
	@echo "Generating doc in PostScript format"
	@echo "-----------------------------------"
	@docbook2ps -d $(TARGET).dsl'#print' $(TARGET).sgml

$(TARGET)/index.html: $(SRC) $(DSL) $(IMAGES)
	@echo ""
	@echo "Generating all HTML pages"
	@echo "-------------------------"
	@rm -fr $(TARGET)
	@docbook2html -d $(TARGET).dsl'#html' -o $(TARGET) $(TARGET).sgml

$(TARGET).html: $(SRC) $(DSL) $(IMAGES)
	@echo ""
	@echo "Generating doc in HTML format"
	@echo "-----------------------------"
	@docbook2html -u -d $(TARGET).dsl'#txt' $(TARGET).sgml
	
$(TARGET).pdf: $(SRC) $(DSL) $(IMAGES)
	@echo ""
	@echo "Generating doc in PDF format"
	@echo "----------------------------"
	@docbook2pdf -d $(TARGET).dsl'#pdf' $(TARGET).sgml

$(TARGET).rtf: $(SRC) $(DSL) $(IMAGES)
	@echo ""
	@echo "Generating doc in RTF format"
	@echo "----------------------------"
	@docbook2rtf -d $(TARGET).dsl'#print' $(TARGET).sgml

clean: 
	@echo ""
	@echo "Cleaning up the documentation directory"
	@echo ""
	@rm -fr $(TARGET).{aux,log,out,tex,dvi,pdf,ps,txt,rtf,html} $(TARGET)

install: all
	@echo ""
	@echo "Delivering mondorescue documentation"
	@echo ""
	@if [ "${INSTALLDIR}" != "" ]; then cp -a $(TARGET).{html,pdf,ps,txt,rtf} $(TARGET) ${INSTALLDIR} ; cp -a ${IMAGESDIR} ${INSTALLDIR}/$(TARGET) ; else echo "No INSTALLDIR specified aborting install"; fi

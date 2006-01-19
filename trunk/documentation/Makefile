TARGET=mondorescue-howto
SRC=$(shell ls *.sgml)
DSL=$(TARGET).dsl
IMAGES=$(shell ls images/*.png | sed 's/\.png/.eps/g')

all: $(TARGET).ps $(TARGET).pdf $(TARGET)/index.html $(TARGET).txt $(TARGET).html $(TARGET).rtf

images/%.eps: images/%.png
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

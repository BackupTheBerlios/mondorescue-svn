TARGET=mondorescue-howto
SRC=$(shell ls *.sgml)
DSL=$(TARGET).dsl
IMAGES=$(shell ls images/*.png)

all: $(TARGET).ps $(TARGET).pdf $(TARGET)/index.html $(TARGET).txt $(TARGET).html
		
$(TARGET).tex: $(SRC) $(IMAGES) $(DSL)
	openjade -t tex -d $(TARGET).dsl'#print' -o $(TARGET).tex $(TARGET).sgml

$(TARGET).txt: $(TARGET).html
	lynx -dump -force_html $(TARGET).html > $(TARGET).txt

$(TARGET).ps: $(TARGET).tex
	hugelatex --interaction nonstopmode -fmt=jadetex -mltex $(TARGET).tex
	hugelatex --interaction nonstopmode -fmt=jadetex -mltex $(TARGET).tex
	hugelatex --interaction nonstopmode -fmt=jadetex -mltex $(TARGET).tex
	mv $(TARGET).tex.dvi $(TARGET).dvi
	dvips -o $(TARGET).ps $(TARGET).dvi

$(TARGET)/index.html: $(SRC) $(DSL) $(IMAGES)
	mkdir -p $(TARGET)
	cd $(TARGET)
	openjade -t sgml -d ../$(TARGET).dsl'#html' ../$(TARGET).sgml
	cd ..

$(TARGET).html: $(SRC) $(DSL) $(IMAGES)
	openjade -t sgml -d ../$(TARGET).dsl'#txt' ../$(TARGET).sgml
	
$(TARGET)pdf.tex: $(SRC) $(IMAGES) $(DSL)
	openjade -t tex -d $(TARGET).dsl'#pdf' -o $(TARGET)pdf.tex $(TARGET).sgml

$(TARGET).pdf: $(TARGET)pdf.tex
	hugepdflatex --interaction nonstopmode -fmt=pdfjadetex -mltex $(TARGET)pdf.tex
	hugepdflatex --interaction nonstopmode -fmt=pdfjadetex -mltex $(TARGET)pdf.tex
	hugepdflatex --interaction nonstopmode -fmt=pdfjadetex -mltex $(TARGET)pdf.tex
	mv $(TARGET)pdf.tex.pdf $(TARGET).pdf

clean: 
	rm -fr $(TARGET).{aux,log,out,tex,dvi,pdf,ps,txt} $(TARGET)

TARGET=mondorescue-howto
SRC=$(shell ls *.sgml)
DSL=$(TARGET).dsl
IMAGES=$(shell ls images/*.png)

all: $(TARGET).ps $(TARGET).pdf $(TARGET)/index.html $(TARGET).txt $(TARGET).html $(TARGET).rtf
		
$(TARGET).txt: $(SRC)
	docbook2txt $(TARGET).sgml

$(TARGET).ps: $(SRC) $(DSL) $(IMAGES)
	#docbook2ps -d $(TARGET).dsl'#print' -o $(TARGET).ps $(TARGET).sgml
	docbook2ps $(TARGET).sgml
	hugelatex -fmt=jadetex -mltex $(TARGET).tex
	hugelatex -fmt=jadetex -mltex $(TARGET).tex
	hugelatex -fmt=jadetex -mltex $(TARGET).tex
	dvips $(TARGET).dvi -o $(TARGET).ps

$(TARGET)/index.html: $(SRC) $(DSL) $(IMAGES)
	rm -f $(TARGET)
	docbook2html -d $(TARGET).dsl'#html' -o $(TARGET) $(TARGET).sgml

$(TARGET).html: $(SRC) $(DSL) $(IMAGES)
	docbook2html -u -d $(TARGET).dsl'#txt' $(TARGET).sgml
	
$(TARGET).pdf: $(SRC) $(DSL) $(IMAGES)
	docbook2pdf -d $(TARGET).dsl'#pdf' $(TARGET).sgml
	hugepdflatex -fmt=jadetex -mltex $(TARGET).tex
	hugepdflatex -fmt=jadetex -mltex $(TARGET).tex
	hugepdflatex -fmt=jadetex -mltex $(TARGET).tex
	dvips $(TARGET).dvi -o $(TARGET).pdf

$(TARGET).rtf: $(SRC) $(DSL) $(IMAGES)
	docbook2rtf -d $(TARGET).dsl'#pdf' $(TARGET).sgml

clean: 
	rm -fr $(TARGET).{aux,log,out,tex,dvi,pdf,ps,txt,rtf} $(TARGET)

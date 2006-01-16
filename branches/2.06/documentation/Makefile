TARGET=mondorescue-howto
SRC=$(shell ls *.sgml)
DSL=$(TARGET).dsl
IMAGES=$(shell ls images/*.png)

all: $(TARGET).ps $(TARGET).pdf $(TARGET)/index.html $(TARGET).txt $(TARGET).html $(TARGET).rtf
		
$(TARGET).txt: $(SRC)
	docbook2txt -u $(TARGET).sgml -o $(TARGET).txt

$(TARGET).ps: $(SRC) $(DSL) $(IMAGES)
	docbook2ps -d $(TARGET).dsl'#print' -o $(TARGET).ps $(TARGET).sgml

$(TARGET)/index.html: $(SRC) $(DSL) $(IMAGES)
	mkdir -p $(TARGET)
	cd $(TARGET)
	docbook2html -d $(TARGET).dsl'#html' $(TARGET).sgml
	cd ..

$(TARGET).html: $(SRC) $(DSL) $(IMAGES)
	docbook2html -u -d $(TARGET).dsl'#txt' -o $(TARGET).html $(TARGET).sgml
	
$(TARGET).pdf: $(SRC) $(DSL) $(IMAGES)
	docbook2html -u -d $(TARGET).dsl'#pdf' -o $(TARGET).pdf $(TARGET).sgml

$(TARGET).rtf: $(SRC) $(DSL) $(IMAGES)
	docbook2rtf -u -d $(TARGET).dsl'#pdf' -o $(TARGET).rtf $(TARGET).sgml

clean: 
	rm -fr $(TARGET).{aux,log,out,tex,dvi,pdf,ps,txt,rtf} $(TARGET)

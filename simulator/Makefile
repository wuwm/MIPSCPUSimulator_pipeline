TARGET=pipeline

$(TARGET): pipeline.o
	gcc pipeline.o -lm -o $(TARGET)
	
pipeline.o: pipeline.c
	gcc -c pipeline.c

.PHONY: clean
clean:
	rm -f $(TARGET) *.o *.bin *.rpt *.exe *.out
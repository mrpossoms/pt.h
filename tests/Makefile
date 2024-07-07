ROOT=..
include $(ROOT)/vars.mk
INC+=-I$(ROOT)/inc
LIB+=-L$(ROOT)/lib/$(TARGET)
CFLAGS+=
LINK+=

TESTS=$(patsubst %.cpp,bin/%,$(wildcard *.cpp))

$(ROOT)/lib/$(TARGET)/lib$(PROJECT).a:
	$(MAKE) -C $(ROOT) static

bin: $(ROOT)/inc/pt.h #$(ROOT)/lib/$(TARGET)/lib$(PROJECT).a
	mkdir -p bin

bin/%: %.cpp bin $(ROOT)/inc/pt.h
	$(CXX) $(CFLAGS) $(LIB) $(INC) $(LIBS) $< -o $@ $(LINK)

.PHONY: test clean 
test: $(TESTS)
	@./.test-runner.py

clean:
	rm -rf bin/



include pin.mk
srcdir := ./
builddir := ./

user_flags := -D_USING_DEBUG_INFO
CXXFLAGS += -fPIC -D_GNU_SOURCE $(user_flags)
INCS += -I$(srcdir)
LDFLAGS += 
CXXFLAGS += -O3 -fomit-frame-pointer
TOOL_LIBS += -lpthread

pincxxgendepend = $(CXX) $(CXXFLAGS) $(TOOL_INCLUDES) $(TOOL_CXXFLAGS) $(INCS) -MM -MT $@ -MF $(builddir)$*.d $<

main.so : main.o
	$(LINKER) $(TOOL_LDFLAGS) $(LINK_DEBUG) ${LINK_EXE}$@ $^ ${TOOL_LPATHS} $(TOOL_LIBS) $(DBG)

main.o : main.cpp main_basictype.h
	@$(pincxxgendepend);
	$(CXX) $(CXXFLAGS) $(TOOL_INCLUDES) $(TOOL_CXXFLAGS) $(INCS) $(COMP_OBJ)$@ $<

VecDetect.o : VecDetect.cpp VecDetect.h main_basictype.h
	@$(pincxxgendepend);
	$(CXX) $(CXXFLAGS) $(TOOL_INCLUDES) $(TOOL_CXXFLAGS) $(INCS) $(COMP_OBJ)$@ $<

clean:
	rm -rf *.o *.so
	


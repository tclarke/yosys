GENFILES += backends/sexpr/sexpr_parser.tab.cc
GENFILES += backends/sexpr/sexpr_parser.tab.hh
GENFILES += backends/sexpr/sexpr_parser.output
GENFILES += backends/sexpr/sexpr_lexer.cc

backends/sexpr/sexpr_parser.tab.cc: backends/sexpr/sexpr_parser.y
	$(Q) mkdir -p $(dir $@)
	$(P) $(BISON) -Wall -Werror -o $@ -d -r all -b backends/sexpr/sexpr_parser $<

backends/sexpr/sexpr_parser.tab.hh: backends/sexpr/sexpr_parser.tab.cc

backends/sexpr/sexpr_lexer.cc: backends/sexpr/sexpr_lexer.l backends/sexpr/sexpr_parser.tab.cc
	$(Q) mkdir -p $(dir $@)
	$(P) flex -o backends/sexpr/sexpr_lexer.cc $<

backends/sexpr/sexpr_parser.tab.o: CXXFLAGS += -DYYMAXDEPTH=10000000

OBJS += backends/sexpr/sexpr.o
OBJS += backends/sexpr/sexpr_parser.tab.o
OBJS += backends/sexpr/sexpr_lexer.o

foo: backends/sexpr/sexpr_lexer.o backends/sexpr/sexpr_parser.tab.o backends/sexpr/sexpr.o
	$(CXX) -o backends/sexpr/foo -g backends/sexpr/sexpr_lexer.o backends/sexpr/sexpr_parser.tab.o backends/sexpr/sexpr.o
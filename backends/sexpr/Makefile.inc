
OBJS += backends/sexpr/sexpr.o

foo: backends/sexpr/test.o
	$(CXX) -o backends/sexpr/foo -g backends/sexpr/test.o
# Parametros alteraveis

CXX = c++
CXXFLAGS = -std=c++14 -g -Wall -Wno-missing-braces -O3
CXXLIBS =
SRC := main.cc ppm.cc\
 spatial/vec.cc spatial/quaternion.cc spatial/intersection.cc spatial/plane.cc\
 spatial/shape/sphere.cc spatial/shape/box.cc spatial/shape/cylinder.cc\
 spatial/shape/polyhedron.cc spatial/shape/transformed_shape.cc\
 spatial/shape/csg_tree.cc
OBJ := $(SRC:%.cc=build/%.o)
DEP := $(SRC:%.cc=deps/%.d)
NAME = raytracing

# Fim dos parametros

ALL := bin/$(NAME)

all default: $(ALL)

$(ALL): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(ALL) $(OBJ) $(CXXLIBS)

build: $(OBJ)
	@:

build/%.o: src/%.cc deps/%.d
	$(CXX) $(CXXFLAGS) -c -o $@ $<

autodeps deps: $(DEP)
	@:

deps/%.d: src/%.cc
	@$(CXX) $(CXXFLAGS) -MM -MT $(@:deps/%.d=build/%.o) -o $@ $<

check test: all
	bin/$(NAME)

.PHONY: clean

clean:
	$(RM) $(OBJ) $(DEP) $(ALL)

.DEFAULT: all

ifeq ($(MAKECMDGOALS),)
TYPES := all
else
TYPES := $(MAKECMDGOALS)
endif

ifneq ($(shell (echo $(TYPES) | grep -oP "(all|default|build|check|test)")),)
-include $(DEP)
endif

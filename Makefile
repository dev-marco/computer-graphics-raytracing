# Parametros alteraveis

CXX = c++
CXXFLAGS = -std=c++14 -g -Wall -Wno-missing-braces -Ofast -fopenmp
CXXLIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui
SRC := main.cc filemanip.cc raytrace.cc\
 graphics/geometry/vec.cc graphics/geometry/quaternion.cc graphics/geometry/intersection.cc\
 graphics/geometry/line.cc graphics/geometry/plane.cc graphics/geometry/parametric.cc\
 graphics/geometry/poisson_disc.cc\
 graphics/shape/sphere.cc graphics/shape/box.cc graphics/shape/cylinder.cc\
 graphics/shape/polyhedron.cc graphics/shape/transformed.cc graphics/shape/csg_tree.cc
OBJ := $(SRC:%.cc=build/%.o)
DEP := $(SRC:%.cc=deps/%.d)
NAME = raytracing

# Fim dos parametros

ALL := bin/$(NAME)

all default: $(ALL)

$(ALL): $(OBJ)
	@mkdir -p $(shell dirname $(shell readlink -m -- $(@)))
	$(CXX) $(CXXFLAGS) -o $(@) $(OBJ) $(CXXLIBS)

build: $(OBJ)
	@:

build/%.o: src/%.cc deps/%.d
	@mkdir -p $(shell dirname $(shell readlink -m -- $(@)))
	$(CXX) $(CXXFLAGS) -c -o $(@) $(<)

autodeps deps: $(DEP)
	@:

deps/%.d: src/%.cc
	@mkdir -p $(shell dirname $(shell readlink -m -- $(@)))
	@$(CXX) $(CXXFLAGS) -MM -MT $(@:deps/%.d=build/%.o) -o $(@) $(<)

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

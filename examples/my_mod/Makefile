MIX = mix
CFLAGS = -g -O3 -ansi -pedantic -Wall -Wextra -Wno-unused-parameter -std=c++17 -march=native

ERLANG_PATH = $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)
CFLAGS += -I$(ERLANG_PATH)

ifneq ($(OS), Windows_NT)
    CFLAGS += -fPIC

    ifeq ($(shell uname), Darwin)
        LDFLAGS += -dynamiclib -undefined dynamic_lookup
    endif
endif

.PHONY: all my_mod clean

all: my_mod

my_mod:
	$(MIX) compile

priv/my_mod.so: src/module.cpp
	$(CXX) $(CFLAGS) -shared $(LDFLAGS) -o $@ src/module.cpp

clean:
	$(MIX) clean
	$(RM) priv/my_mod.so

ERLANG_PATH = $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)
CFLAGS = -g -O2 -ansi -pedantic -Wall -Wextra -I$(ERLANG_PATH) -std=c++2b

ifneq ($(OS), Windows_NT)
    CFLAGS += -fPIC

    ifeq ($(shell uname), Darwin)
	LDFLAGS += -dynamiclib -undefined dynamic_lookup
    endif
endif

expp.so: clean
	$(CC) $(CFLAGS) -shared $(LDFLAGS) -o $@ expp.cpp

clean:
	$(RM) -r expp.so

format:
	clang-format -i src/*.hpp expp.cpp
	mix format

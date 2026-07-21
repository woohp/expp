ERTS_INCLUDE_DIR ?= $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)

NIF_LDFLAGS := -shared
ifeq ($(shell uname -s),Darwin)
NIF_LDFLAGS += -undefined dynamic_lookup
endif

expp.so: expp.cpp
	$(CXX) -I$(ERTS_INCLUDE_DIR) -std=c++23 -fPIC $(NIF_LDFLAGS) -o $@ expp.cpp

clean:
	$(RM) expp.so

format:
	clang-format -i include/expp/*.hpp expp.cpp
	mix format

bundle:
	./scripts/bundle.sh

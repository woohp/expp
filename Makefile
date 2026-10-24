ERTS_INCLUDE_DIR ?= $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)

NIF_LDFLAGS := -shared
ifeq ($(shell uname -s),Darwin)
NIF_LDFLAGS += -undefined dynamic_lookup
endif

HEADERS := $(wildcard include/expp/*.hpp)

expp.so: expp.cpp $(HEADERS)
	$(CXX) -I$(ERTS_INCLUDE_DIR) -Iinclude -std=c++23 -Wall -Wextra -Werror -fPIC $(NIF_LDFLAGS) -o $@ expp.cpp

# Each header must compile on its own, whatever the user includes first.
check-headers:
	@for h in $(HEADERS); do \
		echo '#include "'$$h'"' | $(CXX) -I$(ERTS_INCLUDE_DIR) -std=c++23 -Wall -Wextra -Werror -fsyntax-only -x c++ - || exit 1; \
	done

.PHONY: check-headers clean format

clean:
	$(RM) expp.so

format:
	clang-format -i include/expp/*.hpp expp.cpp
	mix format

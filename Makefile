ERTS_INCLUDE_DIR ?= $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)

expp.so: expp.cpp
	$(CXX) -I$(ERTS_INCLUDE_DIR) -std=c++23 -fPIC -shared -undefined dynamic_lookup -o $@ expp.cpp

clean:
	$(RM) expp.so

format:
	clang-format -i src/*.hpp expp.cpp
	mix format

bundle:
	./scripts/bundle.sh

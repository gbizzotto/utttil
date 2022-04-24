
TYPE = debug

UID := $(shell id -u)
PLATFORM := $(shell uname -s)
DOCKER_BIN := $(shell which docker)

OBJDIR_BASE = obj
BINDIR_BASE = bin

CFGDIR := cfg
SRCDIR := src
OBJDIR = $(OBJDIR_BASE)/$(CXX)_$(TYPE)
BINDIR = $(BINDIR_BASE)/$(CXX)_$(TYPE)
GENDIR = gen
DEPDIR := ..
TESTDIR := tests
DOCKERIMG_BASE_NAME = ttt/build-

CFLAGS_debug = -g -Wall -Wextra -fsanitize=address,leak
CFLAGS_perf = -O3 -DNDEBUG
CFLAGS_release = -g -O3 -fno-omit-frame-pointer -DNDEBUG
EXTRA_CFLAGS = 
CFLAGS = $(CFLAGS_$(TYPE)) $(EXTRA_CFLAGS) --std=c++17 -I$(DEPDIR)/abseil-cpp -I$(DEPDIR)/liburing/src/include -I. -I$(SRCDIR)/ 

LD=$(CXX)
LDFLAGS = -L $(DEPDIR)/liburing/src/
LDLIBS = -pthread -luring -lasan
#-L$(DEPDIR)/abseil-cpp/build/absl/container/ -L$(DEPDIR)/abseil-cpp/build/absl/synchronization/ -L$(DEPDIR)/abseil-cpp/build/absl/time
#-labsl_hashtablez_sampler -labsl_synchronization -labsl_time

TIMEOUT := timeout

ifeq ($(PLATFORM),Darwin)
	CFLAGS += -I/usr/local/opt/openssl/include
	LDLIBS += -L/usr/local/opt/openssl/lib -framework CoreFoundation
	TIMEOUT := gtimeout
endif

# all .d files
D_FILES = $(shell find $(OBJDIR) -name "*.d" 2> /dev/null)
# all cpp files not in the root of src/
all_cpps  = $(shell find $(SRCDIR)/* -type f -name '*.cpp')
all_objs  = $(addprefix $(OBJDIR)/,$(patsubst $(SRCDIR)/%.cpp,%.o,$(all_cpps)))
# all cpp files in the root of src/ and starting with test_
all_tests = $(patsubst $(SRCDIR)/%.cpp,%,$(wildcard $(SRCDIR)/test_*.cpp))
# all cpp files in the root of src/ that don't start with test_
all_bins  = $(filter-out $(all_tests), $(patsubst $(SRCDIR)/%.cpp,%,$(wildcard $(SRCDIR)/*.cpp)))
# hand-crafted lib order
#all_absl_libs = 3rd/abseil-cpp/build/absl/container/libabsl_hashtablez_sampler.a 3rd/abseil-cpp/build/absl/debugging/libabsl_leak_check.a 3rd/abseil-cpp/build/absl/debugging/libabsl_failure_signal_handler.a 3rd/abseil-cpp/build/absl/synchronization/libabsl_synchronization.a 3rd/abseil-cpp/build/absl/debugging/libabsl_stacktrace.a 3rd/abseil-cpp/build/absl/debugging/libabsl_symbolize.a 3rd/abseil-cpp/build/absl/debugging/libabsl_debugging_internal.a 3rd/abseil-cpp/build/absl/debugging/libabsl_leak_check_disable.a 3rd/abseil-cpp/build/absl/debugging/libabsl_demangle_internal.a 3rd/abseil-cpp/build/absl/container/libabsl_raw_hash_set.a 3rd/abseil-cpp/build/absl/time/libabsl_time.a 3rd/abseil-cpp/build/absl/time/libabsl_time_zone.a 3rd/abseil-cpp/build/absl/time/libabsl_civil_time.a 3rd/abseil-cpp/build/absl/strings/libabsl_strings.a 3rd/abseil-cpp/build/absl/strings/libabsl_strings_internal.a 3rd/abseil-cpp/build/absl/strings/libabsl_cord.a 3rd/abseil-cpp/build/absl/strings/libabsl_str_format_internal.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_parse.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_private_handle_accessor.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_program_name.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_internal.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_config.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_commandlineflag.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_usage_internal.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_marshalling.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_usage.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_reflection.a 3rd/abseil-cpp/build/absl/flags/libabsl_flags_commandlineflag_internal.a 3rd/abseil-cpp/build/absl/random/libabsl_random_distributions.a 3rd/abseil-cpp/build/absl/random/libabsl_random_seed_sequences.a 3rd/abseil-cpp/build/absl/random/libabsl_random_internal_randen_slow.a 3rd/abseil-cpp/build/absl/random/libabsl_random_seed_gen_exception.a 3rd/abseil-cpp/build/absl/random/libabsl_random_internal_randen_hwaes.a 3rd/abseil-cpp/build/absl/random/libabsl_random_internal_randen.a 3rd/abseil-cpp/build/absl/random/libabsl_random_internal_pool_urbg.a 3rd/abseil-cpp/build/absl/random/libabsl_random_internal_seed_material.a 3rd/abseil-cpp/build/absl/random/libabsl_random_internal_randen_hwaes_impl.a 3rd/abseil-cpp/build/absl/random/libabsl_random_internal_platform.a 3rd/abseil-cpp/build/absl/random/libabsl_random_internal_distribution_test_util.a 3rd/abseil-cpp/build/absl/numeric/libabsl_int128.a 3rd/abseil-cpp/build/absl/types/libabsl_bad_optional_access.a 3rd/abseil-cpp/build/absl/types/libabsl_bad_variant_access.a 3rd/abseil-cpp/build/absl/types/libabsl_bad_any_cast_impl.a 3rd/abseil-cpp/build/absl/hash/libabsl_city.a 3rd/abseil-cpp/build/absl/hash/libabsl_hash.a 3rd/abseil-cpp/build/absl/status/libabsl_status.a 3rd/abseil-cpp/build/absl/base/libabsl_malloc_internal.a 3rd/abseil-cpp/build/absl/base/libabsl_base.a 3rd/abseil-cpp/build/absl/base/libabsl_strerror.a 3rd/abseil-cpp/build/absl/base/libabsl_raw_logging_internal.a 3rd/abseil-cpp/build/absl/base/libabsl_exponential_biased.a 3rd/abseil-cpp/build/absl/base/libabsl_throw_delegate.a 3rd/abseil-cpp/build/absl/base/libabsl_log_severity.a 3rd/abseil-cpp/build/absl/base/libabsl_dynamic_annotations.a 3rd/abseil-cpp/build/absl/base/libabsl_periodic_sampler.a 3rd/abseil-cpp/build/absl/base/libabsl_scoped_set_env.a 3rd/abseil-cpp/build/absl/base/libabsl_spinlock_wait.a 3rd/abseil-cpp/build/absl/debugging/libabsl_examine_stack.a 3rd/abseil-cpp/build/absl/synchronization/libabsl_graphcycles_internal.a
absl_libs = \
$(DEPDIR)/abseil-cpp/build/absl/container/libabsl_hashtablez_sampler.a \
$(DEPDIR)/abseil-cpp/build/absl/container/libabsl_raw_hash_set.a \
$(DEPDIR)/abseil-cpp/build/absl/hash/libabsl_hash.a \
$(DEPDIR)/abseil-cpp/build/absl/hash/libabsl_city.a \
$(DEPDIR)/abseil-cpp/build/absl/synchronization/libabsl_synchronization.a \
$(DEPDIR)/abseil-cpp/build/absl/debugging/libabsl_stacktrace.a \
$(DEPDIR)/abseil-cpp/build/absl/debugging/libabsl_symbolize.a \
$(DEPDIR)/abseil-cpp/build/absl/debugging/libabsl_debugging_internal.a \
$(DEPDIR)/abseil-cpp/build/absl/debugging/libabsl_demangle_internal.a \
$(DEPDIR)/abseil-cpp/build/absl/time/libabsl_time.a \
$(DEPDIR)/abseil-cpp/build/absl/time/libabsl_time_zone.a \
$(DEPDIR)/abseil-cpp/build/absl/numeric/libabsl_int128.a \
$(DEPDIR)/abseil-cpp/build/absl/base/libabsl_malloc_internal.a \
$(DEPDIR)/abseil-cpp/build/absl/base/libabsl_base.a \
$(DEPDIR)/abseil-cpp/build/absl/base/libabsl_raw_logging_internal.a \
$(DEPDIR)/abseil-cpp/build/absl/base/libabsl_exponential_biased.a \
$(DEPDIR)/abseil-cpp/build/absl/base/libabsl_dynamic_annotations.a \
$(DEPDIR)/abseil-cpp/build/absl/base/libabsl_spinlock_wait.a \
$(DEPDIR)/abseil-cpp/build/absl/synchronization/libabsl_graphcycles_internal.a 

all: $(all_bins) $(all_tests)
	
docker:
	$(if $(findstring $(PLATFORM),Darwin),$(eval ERRMSG=, try brew cask install docker))
	$(if $(findstring $(PLATFORM),Linux),$(eval ERRMSG=, try sudo apt-get install docker))
	$(if $(DOCKER_BIN),,$(error "Please install docker$(ERRMSG)."))
	$(eval COMPILER=$(CXX))
	$(eval VERSION=latest)
	$(if $(findstring -,$(CXX)),$(eval COMPILER=$(firstword $(subst -, ,$(CXX)))))
	$(if $(findstring -,$(CXX)),$(eval VERSION=$(lastword $(subst -, ,$(CXX)))))
	$(if $(findstring $(COMPILER),g++ gcc), \
		$(eval COMPILER = gcc) \
		$(eval URL = https://registry.hub.docker.com/v1/repositories/gcc/tags) \
	)
	$(if $(findstring $(COMPILER),clang clang++), \
		$(eval COMPILER = clang) \
		$(eval URL = https://registry.hub.docker.com/v1/repositories/rsmmr/clang/tags) \
	)
	$(eval VERSIONS = $(shell wget -q $(URL) -O - | sed -e 's/[][]//g' -e 's/"//g' -e 's/ //g' | tr '}' '\n'  | awk -F: '{print $$3"  "}'))
	$(if $(findstring $(VERSION),$(VERSIONS)),,$(error $(COMPILER) version $(VERSION) not found. Available versions: $(VERSIONS)))
	$(eval DOCKERIMG = $(DOCKERIMG_BASE_NAME)$(COMPILER)-$(VERSION))
	$(DOCKER_BIN) build --build-arg VERSION=$(VERSION) -t $(DOCKERIMG) -f $(CFGDIR)/Dockerfile.$(COMPILER) .
	$(DOCKER_BIN) run -u $(UID):$(UID) -it --rm --hostname $@ -v$(CURDIR):/ttt -w/ttt $(DOCKERIMG) $(MAKE) CXX=g++ TYPE=$(TYPE) OBJDIR=$(OBJDIR_BASE)/$(CXX)_$(TYPE) BINDIR=$(BINDIR_BASE)/$(CXX)_$(TYPE)

thorough: 
	$(eval COMPILERS=g++-latest g++-9.3 g++-8.4 clang++-latest clang++-8.0 clang++9.0)
	$(foreach COMPILER,$(COMPILERS),$(MAKE) --no-print-directory CXX=$(COMPILER) EXTRA_CFLAGS='-Wall -Werror' TYPE=debug docker;)

perf: $(TARGET)
	sudo perf record -g bin/$(TARGET)
	sudo perf report -g


loc:
	@git ls-files | grep -E -- 'pp$$|Makefile' | xargs wc -l | tail -n 1

clean:
	rm -rf $(OBJDIR_BASE)
	rm -rf $(BINDIR_BASE)
	rm -rf $(SRCDIR)/headers.hpp.gch
	rm -rf perf.data*

RED=\033[1;31m
GREEN=\033[1;32m
YELLOW=\033[1;33m
NC=\033[0m # No Color

do_%:
	@if [ -f "$(BINDIR_BASE)/$*" ]; then \
		printf '\t%-64s' $*; \
		output=`$(TIMEOUT) 60s $(BINDIR_BASE)/$* 2>&1 >/dev/null`; \
		if [ $$? -ne 0 ]; then \
			echo "$(RED)[FAIL]$(NC)"; \
			echo "$$output"; \
		else \
			echo "$(GREEN)[OK]$(NC)"; \
		fi; \
	else \
		printf '\t%-64s$(YELLOW)[Not found]$(NC)\n' $(bin); \
	fi; \
	
test: $(foreach bin,$(if $(TARGET),$(TARGET),$(all_tests)), do_$(bin))

$(TESTDIR): $(all_tests)
	@#pass

-include $(D_FILES)

deps:
ifeq ($(PLATFORM),Darwin)
	brew install openssl coreutils
endif
	@if [ ! -d $(DEPDIR)"/abseil-cpp" ]; then \
		echo "cloning https://github.com/abseil/abseil-cpp.git into "$(DEPDIR); \
		git clone -q --branch 20200225.2 --depth 1 https://github.com/abseil/abseil-cpp.git $(DEPDIR)/abseil-cpp ; \
		cd $(DEPDIR)/abseil-cpp; \
		mkdir -p build ; \
		cd build ; \
		cmake .. ; \
		$(MAKE) ; \
	fi

gen:
	#pass

-include $(OBJDIR)/headers.d

headers: $(SRCDIR)/headers.hpp.gch

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(SRCDIR)/headers.hpp.gch 
	@mkdir -p $(@D)
	@echo $<
	@$(CXX) $(CFLAGS) -c -MMD -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.cc $(SRCDIR)/headers.hpp.gch
	@mkdir -p $(@D)
	@echo $<
	@$(CC) $(CFLAGS) -c -MMD -o $@ $<

%: $(BINDIR)/%
	@cp $< $(BINDIR_BASE)/$@
	@echo "Available in $(GREEN)$(BINDIR_BASE)/$@$(NC)"

$(SRCDIR)/headers.hpp.gch: $(SRCDIR)/headers.hpp
	@echo Precompiling headers
	@mkdir -p $(OBJDIR)
	@$(CXX) -MMD -MF $(OBJDIR)/headers.d -MT $(SRCDIR)/headers.hpp.gch $(CFLAGS) $<
	@$(CXX) $(CFLAGS) $<

$(BINDIR)/%: $(OBJDIR)/%.o
	$(eval rule := $(shell cat $(OBJDIR)/$*.d))
	$(eval hdrs := $(filter %.hpp,$(rule)))
	$(eval old_objs := $(filter %.o,$(rule)))
	$(eval cpps := $(patsubst $(SRCDIR)/%.hpp,$(SRCDIR)/%.cpp,$(hdrs)))
	$(eval cpps := $(filter $(all_cpps),$(cpps)))
	$(eval objs := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(cpps)))
	$(eval new_objs := $(filter-out $(old_objs),$(objs)))
	$(if $(new_objs), $(shell echo $@: $(new_objs) >> $(OBJDIR)/$*.d))
	$(if $(new_objs), @$(MAKE) --no-print-directory $@)
	$(if $(new_objs), , @echo "Linking $(GREEN)$@$(NC)")
	$(if $(new_objs), , @mkdir -p $(dir $@))
	$(if $(new_objs), , @$(LD) $(LDFLAGS) $(EXTRA_CFLAGS) -o $@ $(OBJDIR)/$*.o $(objs) $(absl_libs) $(LDLIBS))

.PHONY: clean all tests test thorough docker perf loc gen

.PRECIOUS: $(OBJDIR)/%.o
.PRECIOUS: $(BINDIR_BASE)/%
.PRECIOUS: $(BINDIR)/%

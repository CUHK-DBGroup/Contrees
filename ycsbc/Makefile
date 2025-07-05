CC=g++
CFLAGS=-std=c++20 -I./ -DNDEBUG -O3
WFLAGS=-Wall
LDFLAGS=-lpthread
SUBDIRS=
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
WORKLOADS_PATH=$(wildcard workloads/*.spec)
WORKLOADS=$(WORKLOADS_PATH:workloads/%.spec=%)
EXEC=ycsbc

all: $(EXEC)

$(EXEC): $(wildcard *.cc) $(OBJECTS)
	$(CC) $(CFLAGS) $(WFLAGS) $^ $(LDFLAGS) -o $@

gen: $(EXEC) | export_dir
	@for workload in $(WORKLOADS); do \
		echo "Generating $$workload"; \
		./$(EXEC) -db export -P $$workload; \
	done

export_dir:
	@mkdir -p export

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
	@$(RM) $(EXEC)
	@$(RM) -r export

.PHONY: $(SUBDIRS) $(EXEC)

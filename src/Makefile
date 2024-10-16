# Use https://makefiletutorial.com for makefile rules and help
# Add targets for new file compilations as needed

OBJECTS := tinyfile_service.o tinyfile_client.o
# OBJECTS := 

all: libsnappyc.so $(OBJECTS)

$(OBJECTS): %.o: %.c
	# gcc $^ -o $@
	gcc $^ tinyfile_library.c -o $@ -L./ -lsnappyc

# Build snappyc_test.o, dependent on libsnappyc.so symlink

# Create symlink of libsnappyc.so.1 and 
# set library path variable
libsnappyc.so: libsnappyc.so.1
	ln -sf ./libsnappyc.so.1 ./libsnappyc.so
	@LD_LIBRARAY_PATH=$LD_LIBRARAY_PATH:./

# Build snappy-c library and move to curr dir
libsnappyc.so.1:
	@cd snappy-c; make clean; make; \
	mv ./libsnappyc.so.1 ../


# Files to clean, add additional output files as needed
CLEAN := snappyc_test.o tinyfile_library.o tinyfile_service.o libsnappyc.so libsnappyc.so.1

# Removes all files specified by CLEAN
clean: 
	rm -f ${CLEAN}
	rm -f ${OBJECTS}
# Uncomment line below if removing output dir
	@#@ rm -r ../bin/output
	@cd snappy-c; make clean

# Run snappyc_test compression/uncompression
run:
	./run.sh
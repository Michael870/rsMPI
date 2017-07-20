FLAGS= -w -fPIC -g -c #-DDEBUG
CFLAGS= -shared -Wl,-soname,liblsmpi.so.1 -lrdmacm
_OBJ = shadow_leap.o collectives_opt.o mpi_override.o shadow_coordinate.o signal_handler.o shared.o monitor_thread.o socket.o msg_queue.o request_list.o 
LDIR=../lib
ODIR=../obj
OBJ= $(patsubst %,$(ODIR)/%,$(_OBJ))

all: $(LDIR)/liblsmpi.so.1.0

$(LDIR)/liblsmpi.so.1.0: $(OBJ)
	gcc $(CFLAGS) -o $@ $^ 
	ln -sf $@ $(LDIR)/liblsmpi.so.1
	ln -sf $@ $(LDIR)/liblsmpi.so
	ctags -R -f tags 

$(ODIR)/mpi_override.o: mpi_override.c mpi_override.h shadow_coordinate.h shared.h 
	mpicc $(FLAGS) -o $@  $<

$(ODIR)/shadow_coordinate.o: shadow_coordinate.c shadow_coordinate.h mpi_override.h
	mpicc $(FLAGS) -o $@ $<

$(ODIR)/collectives_opt.o: collectives_opt.c mpi_override.h 
	mpicc $(FLAGS) -o $@  $<

$(ODIR)/request_list.o: request_list.c request_list.h mpi_override.h
	mpicc $(FLAGS) -o $@  $<

$(ODIR)/signal_handler.o: signal_handler.c mpi_override.h shared.h
	mpicc $(FLAGS) -o $@  $<

$(ODIR)/shared.o: shared.c shared.h mpi_override.h
	mpicc $(FLAGS) -o $@  $<

$(ODIR)/collectives_linear.o: collectives_linear.c mpi_override.h shadow_coordinate.h shared.h request_list.h 
	mpicc $(FLAGS) -o $@  $<

$(ODIR)/error_handler.o: error_handler.c error_handler.h mpi_override.h
	mpicc $(FLAGS) -o $@  $<
 
$(ODIR)/shadow_leap.o: shadow_leap.c shadow_leap.h mpi_override.h shared.h
	mpicc $(FLAGS) -o $@  $<

$(ODIR)/topo_list.o: topo_list.c topo_list.h
	mpicc $(FLAGS) -o $@ $<

$(ODIR)/monitor_thread.o: monitor_thread.c monitor_thread.h mpi_override.h
	mpicc $(FLAGS) -o $@ $<

$(ODIR)/socket.o: socket.c socket.h mpi_override.h
	mpicc $(FLAGS) -o $@ $<

$(ODIR)/msg_queue.o: msg_queue.c msg_queue.h mpi_override.h
	mpicc $(FLAGS) -o $@ $<







clean:
	rm -f $(LDIR)/liblsmpi.so*
	rm -f $(ODIR)/*.o
	rm tags

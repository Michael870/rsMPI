CXX= mpicc 
LSLIB= -L../lib -llsmpi -lmpi 

PRG= hello_world ring order send_recv loops
RELIABLE_PRG= rhello_world rring rorder rsend_recv rloops

all: $(PRG) $(RELIABLE_PRG)

hello_world: hello_world.c 
	$(CXX) $^  -o $@

rhello_world: hello_world.c 
	$(CXX) $(LSLIB) $^ -o $@

send_recv: send_recv.c 
	$(CXX) $^  -o $@

rsend_recv: send_recv.c 
	$(CXX) $(LSLIB) $^ -o $@


ring: ring.c 
	$(CXX) $^  -o $@

rring: ring.c 
	$(CXX) $(LSLIB) $^ -o $@

order: order.c 
	$(CXX) $^  -o $@

rorder: order.c 
	$(CXX) $(LSLIB) $^ -o $@

loops: loops.c
	$(CXX) $^ -o $@

rloops: loops.c
	$(CXX) $(LSLIB) $^ -o $@

clean:
	rm -f $(PRG) $(RELIABLE_PRG)

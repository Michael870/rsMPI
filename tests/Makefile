CXX= mpicc 
LSLIB= -L../lib -llsmpi -lmpi -lpthread 

PRG= gatherv_test gather_test allgather_test allgatherv_test allreduce_test alltoall_test alltoallv_test anysource_test barrier_test bcast_test reduce_test probe_test send_test recv_test scatter_test scatterv_test scan_test reduce_scatter_test 
RELIABLE_PRG= rgatherv_test rgather_test rallgather_test rallgatherv_test rallreduce_test ralltoall_test ralltoallv_test ranysource_test rbarrier_test rbcast_test rreduce_test rprobe_test rsend_test rrecv_test rscatter_test rscatterv_test rscan_test rreduce_scatter_test

all: $(PRG) $(RELIABLE_PRG)

reduce_scatter_test: reduce_scatter_test.c
	$(CXX) $^ -o $@

rreduce_scatter_test: reduce_scatter_test.c
	$(CXX) $(LSLIB) $^ -o $@


scan_test: scan_test.c
	$(CXX) $^ -o $@

rscan_test: scan_test.c
	$(CXX) $(LSLIB) $^ -o $@

scatterv_test: scatterv_test.c
	$(CXX) $^ -o $@

rscatterv_test: scatterv_test.c
	$(CXX) $(LSLIB) $^ -o $@


scatter_test: scatter_test.c
	$(CXX) $^ -o $@

rscatter_test: scatter_test.c
	$(CXX) $(LSLIB) $^ -o $@

probe_test: probe_test.c
	$(CXX) $^ -o $@

rprobe_test: probe_test.c
	$(CXX) $(LSLIB) $^ -o $@


gatherv_test: gatherv_test.c
	$(CXX) $^ -o $@

rgatherv_test: gatherv_test.c
	$(CXX) $(LSLIB) $^ -o $@

gather_test: gather_test.c
	$(CXX) $^ -o $@

rgather_test: gather_test.c
	$(CXX) $(LSLIB) $^ -o $@

bcast_test: bcast_test.c 
	$(CXX) $^  -o $@

rbcast_test: bcast_test.c 
	$(CXX) $(LSLIB) $^ -o $@

allgather_test: allgather_test.c 
	$(CXX) $^  -o $@

rallgather_test: allgather_test.c 
	$(CXX) $(LSLIB) $^ -o $@

allgatherv_test: allgatherv_test.c 
	$(CXX) $^  -o $@

rallgatherv_test: allgatherv_test.c 
	$(CXX) $(LSLIB) $^ -o $@

allreduce_test: allreduce_test.c 
	$(CXX) $^  -o $@

rallreduce_test: allreduce_test.c 
	$(CXX) $(LSLIB) $^ -o $@

alltoall_test: alltoall_test.c 
	$(CXX) $^  -o $@

ralltoall_test: alltoall_test.c 
	$(CXX) $(LSLIB) $^ -o $@

alltoallv_test: alltoallv_test.c 
	$(CXX) $^  -o $@

ralltoallv_test: alltoallv_test.c 
	$(CXX) $(LSLIB) $^ -o $@

anysource_test: anysource_test.c 
	$(CXX) $^  -o $@

ranysource_test: anysource_test.c 
	$(CXX) $(LSLIB) $^ -o $@


barrier_test: barrier_test.c 
	$(CXX) $^  -o $@

rbarrier_test: barrier_test.c 
	$(CXX) $(LSLIB) $^ -o $@

reduce_test: reduce_test.c 
	$(CXX) $^  -o $@

rreduce_test: reduce_test.c 
	$(CXX) $(LSLIB) $^ -o $@

send_test: send_test.c
	$(CXX) $^ -o $@

rsend_test: send_test.c
	$(CXX) $(LSLIB) $^ -o $@

recv_test: recv_test.c
	$(CXX) $^ -o $@

rrecv_test: recv_test.c
	$(CXX) $(LSLIB) $^ -o $@


clean:
	rm -f $(PRG) $(RELIABLE_PRG)

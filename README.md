experiment
===

baseline
---

Edit Makefile, set MEL_BASELINE = 1
make clean
make
./app.out


tx oblivious
---

Edit Makefile, set MEL_BASELINE = 0
make clean
make
./app.out




real run
---

```
make 
# make SGX_MODE=HW
# /opt/intel/sgxsdk/bin/x64/sgx_sign sign -key Enclave/Enclave_private.pem -enclave enclave.so -out enclave.signed.so -config Enclave/Enclave.config.xml 
./app
```

misc
---

Purpose of SampleEnclave

The project demonstrates several fundamental usages:

    1. Initializing and destroying an enclave
    2. Creating ECALLs or OCALLs
    3. Calling trusted libraries inside the enclave


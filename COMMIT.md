execution flow
----

```
<------- UDF -------->|<--------Intel SDK---------->|<----------UDF----------->

 App.cpp   |                                                 | Enclave.cpp
           | edl      |                             | edl    |
           |  ec:proxy|                             |  bridge|
           |  oc:bridge                             |  proxy |
           |          | sgx_urts |sgx_trts/svc/crypt|        |
           |          |          |                  |        |
                                 |
       app/lib.so                |              enclave.so
                                 |
---------------------------------+---------------------------------------------
main()     |          |          |                  |        |
      foo1 |          |          |                  |        |
           |ec_foo    |          |                  |        |
           |          |sgx_ec(1,oc_t_E,ms)          |        |
           |          |          |enc_entry{}       |        |           
           |          |          |             do_ec|        |           
           |          |          |                  |<g_ecall_table[1]>  
           |          |          |                  |sgx_ec_foo(ms)      
           |          |          |                  |        |ec_foo -----+
           |          |          |enc_entry{EEXIT}  |        |            |
ret   ret  |ret       |ret       |                  |        |            | 
---------------------------------+----------------------------------------v---ocall
           |          |          |                  |        |            bar1 
           |          |          |                  |oc_bar  |           
           |          |          |   sgx_ocloc()->ms|        |           
           |          |          |      sgx_oc(0,ms)|        |           
           |          |          |do_oc{EEXIT}      |        |           
           |<oc_t_E[0]>          |                  |        |           
           |E_oc_bar(ms)         |                  |        |           
oc_bar     |          |          |                  |        |           
```

* `ms` refers to marshalling region, a host memory area used for passing context for ocall/ecall

Coding ecall `int ecall_foo(int i)`
---

| ec_malloc | u | t |
| --- | --- | --- |
| inv |App.cpp |Enclave_t.c<=edl|
| |`ret=ecall_foo(global_eid,&retval,i);`|`ms->ms_retval = ecall_foo(ms->ms_i);`|
| del |Enclave_u.h<=edl|Enclave_t.h<=edl|
||`int ecall_foo(int i);`|`int ecall_foo(int i);`|
| def |Enclave_u.c<=edl| Enclave.cpp |
||`sgx_status_t ecall_foo(eid,retval,int i)`|`int ecall_foo(int i)`|

ecall/ocall control workflow
---

```
main()@App/App.cpp
  +-->initialize_enclave()@App/App.cpp
  |     +-->sgx_create_enclave()@libsgx_urts.so
  |
  +-->edger8r_function_attributes()@App/Edger8rSyntax/Functions.cpp
  |     +-->ecall_function_calling_convs(eid)@App/Enclave_u.c(Enclave/Enclave.edl)
  |       ...ecall...
  |
  |         @Enclave:sgx_ecall_function_calling_convs(pms)@Enclave/Enclave_t.c(Enclave/Edger8rSyntax/Functions.edl)
  |            +-->ecall_function_calling_convs()@Enclave/Edger8rSyntax/Functions.cpp
  |         
  +-->ecall_foo1()@App/TrustedLibrary/Libc.cpp,@App/App.h
        +-->ecall_foo(eid)@App/Enclave_u.c(Libc.edl by ecall_foo())
          +-->sgx_ecall(eid,27,...)@sgxsdk/lib64/libsgx_urts.so,include/sgx_edger8r.h
            ...g_ecall_table[27==>sgx_ecall_foo]@Enclave/Enclave_t.c,Enclave.edl

              @enclave.so:sgx_ecall_foo(pms)@Enclave/Enclave_t.c
                +-->ecall_foo()@Enclave/TrustedLibrary/Libc.cpp
---------------------------------ocall----------------------------------------------------------
                  +-->malloc()@enclave.so,/opt/intel/sgxsdk/lib64/libsgx_trts.a
                  +-->bar1(const char *,...)@Enclave/Enclave.cpp
                    +-->
                    ...ocall_bar([in, string] const char *str)@Enclave/Enclave.edl

                    @app:ocall_bar(const char *)@App/App.cpp
```

* `sgx_create_enclave()@urts.so` returns a launch token

about ecall-lib `int ecall_foo(int i)`:
---

* del: `public int ecall_foo(int i);`
    * `Enclave.edl,Enclave_u.h`
* def: `int ecall_foo(int i)`
    * Enclave.cpp
* inv: `ret = ecall_foo(global_eid, &retval, i);`
    * App.cpp

about ecall-app `int ecall_foo1(int i)`
---

* def: `void ecall_foo1(int i)`
    * ./App.cpp:
* inv: `ecall_foo1(i);`
    * ./App/App.cpp:    
* del: `void ecall_foo1(int);`
    * ./App/App.h:

about ocall-app `bar1`
---

* def: `void bar1(const char *fmt, ...)`
    * ./Enclave/Enclave.cpp
* inv: `bar1(pointer);`
    * ./Enclave.cpp
* del: `void bar1(const char *fmt, ...);`
    * ./Enclave/Enclave.h

about ocall-lib `ocall_bar(buf)`
---

* def: `void ocall_bar(const char *str)`
    * ./App/App.cpp
* inv: `ocall_bar(buf);`
    * ./Enclave/Enclave.cpp
* del: `void ocall_bar([in, string] const char *str);`
    * ./Enclave/Enclave.edl


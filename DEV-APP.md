Interface Design
===

Argument style
----

pass by pointer

* original: `foo(){ void* ptr = data; ... }` 
* enclave: `foo1(){ ec_foo(&data); }`
    * `ec_foo(long vaddr)` @App.cpp
        * `ec_foo(long vaddr){ void* ptr = (void *) vaddr; ... }` @Enclave.cpp

pass by value

* original: `foo(){ int[] data; ... data[0];}` 
* enclave: `foo(){ ec_foo(data); }`
    * `ec_foo(int data[10])` @App.cpp
        * `ec_foo(int data[10]){ ... data[0];}` @Enclave.cpp


The case of leveldb iterator (per `next()` partitioning)
---

`U` labels untrusted memory, `T` lables Enclave.

```cpp

U: Elem next(Iterator* it){...;SYSCALL;...;}

merge(Iterator* it){
    ...
T:  next(it);
    ...
}

main(){
    Iterator* it;
U:  merge(it);
}
```

==>

```cpp
/////////// @Enclave.cpp /////////////

oc_next(long addr_it) @Enclave.cpp
{
    oc_next(vaddr_it); @App.cpp
}

ec_merge(long vaddr_it) @Enclave.cpp
{
    Iterator* it = vaddr_it;
    ...
    oc_next(it);
    ...
}   

/////////// @App.cpp /////////////
oc_next(long vaddr_it) @App.cpp
{
   Iterator* it = vaddr_it;
   ...
   SYSCALL;
   ...
}

ec_merge(long vaddr_it) @ App.cpp
{
   ec_merge(vaddr_it); @Enclave.cpp
}
   
main(){
   Iterator* it;
   ec_merge(it);
}
```

| code-partition | argument pass-by| ocall freq. | copy # |
| --- | --- | --- | --- |
| `it->next` | value | per `next()` | >=3 |
| `SYSCALL` | value | per `syscall` | >=3 |
| `it->next` | pointer | per `next()` | 1 |
| `SYSCALL` | pointer | per `syscall` | 1 |

Partitioning
===

The case of secure-channel establishment
---

```cpp
class secureChannel_AliceToBob{
  void setup(){
    SSL_new(socket);
  }
  
  void communicate(plaintext{S}){
    SSL_write(plaintext);
    plaintext2 = SSL_read();
  }
}

void SSL_new(){
T: public_key_Bob=CA.getKey("Bob");
T: <public_key_Alice, private_key_Alice{S}>=ssl_genkey_RSA();
T: key_session{S}=ssl_genkey_ATS();
}

SSL_write(message plaintext{S}){
  T: digest=sha1(plaintext,40);
  T: sig=ssl_MAC_sign(plaintext,digest,key_session);
  T: ciphertext=ssl_aes_enc(plaintext,key_session);
  U: ssl_send(ciphertext||sig, Bob);
}

SSL_read(){
  U: <ciphertext,sig>=ssl_recv();
  T: plaintext=ssl_aes_dec(ciphertext,key_session);
  if(ssl_MAC_verify(plaintext,sig,key_session)){
    return plaintext;
  }
}
```

Misc issues
---

1. clear data before `free()`
2. in-Enclave key management
3. `syscall` while sensitive data?
    * e.g. load existing secret key from `*.pem`
    * random number generator (from `/dev/rand/` in Unix)
4. interface: prepended compilation
    * ``SGXP gcc src/main.c``
    * ``./a.out``

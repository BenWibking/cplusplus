// RUN: %cxx --target=spirv32 -verify -fsyntax-only %s

// expected-error@+1 {{volatile-qualified objects are not supported by the device profile}}
volatile int counter;

// expected-error@+1 {{a kernel parameter binding requires a kernel declaration}}
void ordinary([[cxx::global_id]] unsigned int index);

extern "C" [[cxx::kernel]] void entry([[cxx::global_id]] unsigned int index);
extern "C" void entry(unsigned int index) {}

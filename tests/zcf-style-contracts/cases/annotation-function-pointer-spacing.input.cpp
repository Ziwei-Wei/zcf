#define _In_reads_(size)
#define ZCF_STDCALL
using Result = long;
typedef Result(ZCF_STDCALL* Callback)(_In_reads_(size) const void* data, int size);

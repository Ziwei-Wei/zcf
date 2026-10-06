#define _In_reads_(size)
#define WFORMAT_STDCALL
using Result = long;
typedef Result(WFORMAT_STDCALL* Callback)(_In_reads_(size) const void* data, int size);

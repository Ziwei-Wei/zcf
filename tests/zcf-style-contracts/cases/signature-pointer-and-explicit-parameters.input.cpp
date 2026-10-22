typedef Result(ZCF_STDCALL *Callback)(const void *data, int size);
void take(int (*compare)(const void *, const void *));
template <bool Strict> struct Flag {
  explicit(Strict) Flag(int value);
};

int first;
#if TWO_DEFINITIONS
void Alpha() {}
void Beta() {}
#elif NESTED
#if INNER
struct Inner { int value; };
#endif
int afterInner;
#else
int fallback();
#endif
int middle;
#ifdef DECLARATIONS_ONLY
int declaredX();
int declaredY();
#endif
int last;
// Tracing helpers.
#if TRACE
void Trace() {}
#endif
class Widget {
public:
#if WIDGET_DEBUG
  void Dump() {}
#endif
  int state;
};

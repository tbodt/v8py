
#ifdef _WIN32
// v8.h(1680): error C2059: syntax error: '('
#undef COMPILER
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif
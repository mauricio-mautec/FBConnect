# FBConnect
a C++ class implementing to serve as a bridge to the new Interfaces in Firebird 3 API

# Environment setup
- Linux Tumbleweed
- gcc 7.2.1
- valgrind 3.13
- GNU gdb 8.0.1
- Firebird 3

# 2018/01/17 - MAURICIO JUNQUEIRA
- It is in its initial development.
- To test what is already done, execute ./FBConnectTest.sh
- Requirements are a Firebird 3 Server, gcc and valgrind.

# Messages ready so far:
 * int Start (char readwrite)
 * int Commit (void)
 * int CommitRetaining (void)

# Messages in development:
 * Fetch
 * Select
 * Execute
 * ExecuteBind

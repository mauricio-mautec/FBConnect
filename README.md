# FBConnect
a C++ class implementing to serve as a bridge to the new Interfaces in Firebird 3 API

# 2018/01/17 - MAURICIO JUNQUEIRA
- The is in its initial development.
- To test what is alread done, execute ./FBConnectTest.sh
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

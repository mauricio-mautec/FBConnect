# FBConnect 5.0
C++ wrapper class for the Firebird 5.x Object-Oriented API (`<firebird/Interface.h>`).

## Features Ready
- **Connection & Session Management (Firebird 5.x)**
  - `FBConnect(const char *banco, const char *user, const char *senha, const char *charset = "UTF8")`
  - `string getServerVersion()`
  - `bool Ping()`
  - `int Reconnect()`
- **Transactions (Read Committed Read Consistency / Rec Version)**
  - `int Start(char readwrite)` (`FBREAD` / `FBWRITE`)
  - `int Commit()`
  - `int CommitRetaining()`
  - `int Rollback()`
- **Queries, Parameterized Selects & Data Retrieval**
  - `int Select(const char *stmt)` (supports `SELECT` and `INSERT/UPDATE/DELETE ... RETURNING`)
  - `int SelectBind(const char *stmt, const char **dados, int dimensao)`
  - `int Fetch()`
  - `string getColumn(int col)` / `string getColumn(const char *colName)` / `string getColumn(int tam, int col)`
  - `long getInt(int col)` / `long getInt(const char *colName)`
  - `int64_t getInt64(int col)` / `int64_t getInt64(const char *colName)`
  - `double getDouble(int col)` / `double getDouble(const char *colName)`
  - `bool getBool(int col)` / `bool getBool(const char *colName)`
  - `vector<string> getRowVector()`
  - `int getRow(unsigned char *buf, size_t bufSize)` (the overload without `bufSize` is deprecated)
  - `map<string, string> getRowMap()`
  - `int isNull(int col)` / `int isNull(const char *colName)`
  - `unsigned getCount()`
  - `const char* getColumnName(int col)`
  - `int getColumnIndex(const char *colName)`
  - `int getDataSize(int col)`
- **DDL, DML, Parameterized & Batch Execution**
  - `int Execute(const char *stmt)`
  - `int ExecuteBind(const char *stmt, const char **dados, int dimensao)`
  - `int ExecuteBatch(const char *stmt, const vector<vector<string>> &rows)`
  - `int ExecuteBatch(const char *stmt, const vector<vector<const char*>> &rows)` (a `NULL` pointer writes SQL `NULL`; declare the vector type explicitly, a bare brace list is ambiguous)
  - Parameters are sent as text and may hold at most 32765 bytes each; use a BLOB for larger values
  - `uint64_t getAffectedRows()`
  - `int64_t getNextSequence(const char *seqName, int step = 1)`
- **BLOB Read & Write (`IBlob`)**
  - `int isBlob(int col)`
  - `int blobType(int col)`
  - `int workBlob(unsigned char *bid)`
  - `int getBlobSize(IBlob *handler)`
  - `string GetTextBlob()` / `int GetTextBlob(unsigned char *buf, size_t bufSize)` (truncates and sets `ErrorMsg` when the BLOB does not fit; the overload without `bufSize` is deprecated)
  - `int GetFileBlob(FILE *file)`
  - `int SetTextBlob(const char *query, const char *texto, int bloblen)`
  - `int SetFileBlob(const char *query, FILE *file, int bloblen)`
- **Firebird 5.x Native Events (`IAttachment::queEvents` / `IEvents` / `IEventCallback`)**
  - `int EventInit(const char *event, void (*funcao)(unsigned char*, short, unsigned char*) = NULL)`
  - `int EventInit(const vector<string> &events, void (*funcao)(unsigned char*, short, unsigned char*) = NULL)`
  - `int EventInit(const char *event, unsigned char *resultbuff, unsigned char *eventbuff, void (*funcao)(unsigned char*, short, unsigned char*))`
  - `int EventWait(bool block = true, int timeoutMs = 0)`
  - `unsigned int EventCounts()`
  - `unsigned int getEventCount(int index = 0)` / `unsigned int getEventCount(const char *eventName)`
  - `void EventClear()` / `void EventsClear()`
  - `int PostEvent(const char *eventName)`

## Supported Firebird 5.x SQL Data Types
- `SMALLINT`, `INTEGER`, `BIGINT`, `INT128`
- `NUMERIC(p, s)` / `DECIMAL(p, s)` (up to 38 digits precision)
- `FLOAT`, `DOUBLE PRECISION`, `DECFLOAT(16)`, `DECFLOAT(34)`
- `CHAR(n)`, `VARCHAR(n)` (UTF8-aware)
- `BOOLEAN`
- `DATE`, `TIME`, `TIMESTAMP`
- `TIME WITH TIME ZONE`, `TIMESTAMP WITH TIME ZONE` (with IANA regions)
- `BLOB SUB_TYPE TEXT` / `BLOB SUB_TYPE BINARY`

## Build & Run (Test Suite & Example with MauWin)
The test runner and the example use the `MauWin` class from the
`cppClass` repository, expected at `../cppClass-master`
(override with `make MAUWIN_DIR=/path/to/cppClass`). Requires `libfbclient`
and `libncursesw`.

Connection settings come from the environment, never from the source:

| Variable | Meaning |
|---|---|
| `FBCONNECT_USER` / `FBCONNECT_PASS` | Required credentials |
| `FBCONNECT_DB` | `host:/path/db.fdb` (default `127.0.0.1:/var/lib/firebird/data/mautec.fdb`) |
| `FBCONNECT_DB_AUX` | Optional alternate path used for events when the Docker host port blocks the aux port |

Copy `.fbconnect.env.example` to `.fbconnect.env` (git-ignored); `test.sh` and
`FBConnectTest.sh` load it automatically. Async events need the Firebird
`RemoteAuxPort` (e.g. 3051) published by the container alongside 3050.

> **Warning:** test `t000` recreates and reseeds every table in the target
> database. Point `FBCONNECT_DB` at a disposable database.

```bash
cp .fbconnect.env.example .fbconnect.env   # then edit
. ./.fbconnect.env
make
./FBConnectTest          # Interactive MauWin test runner (select individual tests or run all)
./FBConnectTest --batch  # Non-interactive batch mode for scripts/CI
./exemploFBConnect       # Interactive MauWin example application (queries, JOINs, BLOBs, Batch, FB 5.0 types)
```

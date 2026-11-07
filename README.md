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
  - `uint64_t getAffectedRows()`
  - `int64_t getNextSequence(const char *seqName, int step = 1)`
- **BLOB Read & Write (`IBlob`)**
  - `int isBlob(int col)`
  - `int blobType(int col)`
  - `int workBlob(unsigned char *bid)`
  - `int getBlobSize(IBlob *handler)`
  - `string GetTextBlob()` / `int GetTextBlob(unsigned char *buf)`
  - `int GetFileBlob(FILE *file)`
  - `int SetTextBlob(const char *query, const char *texto, int bloblen)`
  - `int SetFileBlob(const char *query, FILE *file, int bloblen)`

## Supported Firebird 5.x SQL Data Types
- `SMALLINT`, `INTEGER`, `BIGINT`, `INT128`
- `NUMERIC(p, s)` / `DECIMAL(p, s)` (up to 38 digits precision)
- `FLOAT`, `DOUBLE PRECISION`, `DECFLOAT(16)`, `DECFLOAT(34)`
- `CHAR(n)`, `VARCHAR(n)` (UTF8-aware)
- `BOOLEAN`
- `DATE`, `TIME`, `TIMESTAMP`
- `TIME WITH TIME ZONE`, `TIMESTAMP WITH TIME ZONE` (with IANA regions)
- `BLOB SUB_TYPE TEXT` / `BLOB SUB_TYPE BINARY`

## Build & Run Tests
```bash
make
./FBConnectTest
```

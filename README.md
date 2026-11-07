# FBConnect 5.0
C++ wrapper class for the Firebird 5.x Object-Oriented API (`<firebird/Interface.h>`).

## Features Ready
- **Connection & Session Management (Firebird 5.x)**
  - `FBConnect(const char *banco, const char *user, const char *senha, const char *charset = "UTF8")`
  - `string getServerVersion()`
- **Transactions (Read Committed Read Consistency / Rec Version)**
  - `int Start(char readwrite)` (`FBREAD` / `FBWRITE`)
  - `int Commit()`
  - `int CommitRetaining()`
  - `int Rollback()`
- **Queries & Data Retrieval**
  - `int Select(const char *stmt)`
  - `int Fetch()`
  - `string getColumn(int col)` / `string getColumn(int tam, int col)`
  - `int isNull(int col)`
  - `unsigned getCount()`
  - `const char* getColumnName(int col)`
  - `int getDataSize(int col)`
- **DDL, DML & Parameterized Statements**
  - `int Execute(const char *stmt)`
  - `int ExecuteBind(const char *stmt, const char **dados, int dimensao)`
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

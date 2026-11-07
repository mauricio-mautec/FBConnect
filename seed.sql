-- Script de criacao e carga de dados de teste para mautec.fdb (Firebird 3.0+)

RECREATE TABLE DEPARTMENT (
    DEPT_NO    CHAR(3) NOT NULL PRIMARY KEY,
    DEPARTMENT VARCHAR(40) NOT NULL,
    HEAD_DEPT  CHAR(3),
    BUDGET     NUMERIC(12,2),
    LOCATION   VARCHAR(30)
);

RECREATE TABLE JOB (
    JOB_CODE        VARCHAR(5) NOT NULL,
    JOB_GRADE       SMALLINT NOT NULL,
    JOB_COUNTRY     VARCHAR(15) NOT NULL,
    JOB_TITLE       VARCHAR(35) NOT NULL,
    MIN_SALARY      NUMERIC(10,2),
    MAX_SALARY      NUMERIC(10,2),
    JOB_REQUIREMENT BLOB SUB_TYPE TEXT,
    PRIMARY KEY (JOB_CODE, JOB_GRADE, JOB_COUNTRY)
);

RECREATE TABLE EMPLOYEE (
    EMP_NO     INTEGER NOT NULL PRIMARY KEY,
    FIRST_NAME VARCHAR(20) NOT NULL,
    LAST_NAME  VARCHAR(25) NOT NULL,
    DEPT_NO    CHAR(3) NOT NULL,
    JOB_CODE   VARCHAR(5) NOT NULL,
    JOB_GRADE  SMALLINT NOT NULL,
    HIRE_DATE  DATE NOT NULL,
    SALARY     NUMERIC(10,2) NOT NULL
);

RECREATE TABLE PROJECT (
    PROJ_ID     CHAR(5) NOT NULL PRIMARY KEY,
    PROJ_NAME   VARCHAR(40) NOT NULL,
    TEAM_LEADER INTEGER,
    PRODUCT     VARCHAR(20),
    PROJ_DESC   BLOB SUB_TYPE TEXT
);

RECREATE TABLE EMPLOYEE_PROJECT (
    EMP_NO  INTEGER NOT NULL,
    PROJ_ID CHAR(5) NOT NULL,
    PRIMARY KEY (EMP_NO, PROJ_ID)
);

RECREATE TABLE SALARY_HISTORY (
    EMP_NO         INTEGER NOT NULL,
    CHANGE_DATE    TIMESTAMP NOT NULL,
    UPDATER_ID     VARCHAR(20) NOT NULL,
    OLD_SALARY     NUMERIC(10,2) NOT NULL,
    PERCENT_CHANGE DOUBLE PRECISION NOT NULL
);

RECREATE TABLE CUSTOMER (
    CUST_NO       INTEGER NOT NULL PRIMARY KEY,
    CUSTOMER      VARCHAR(35) NOT NULL,
    CONTACT_FIRST VARCHAR(20),
    CONTACT_LAST  VARCHAR(20),
    PHONE_NO      VARCHAR(20),
    CITY          VARCHAR(30),
    COUNTRY       VARCHAR(20)
);

RECREATE TABLE TYPE_SHOWCASE (
    ID           INTEGER NOT NULL PRIMARY KEY,
    CODE_FIXED   CHAR(10),
    NAME_VAR     VARCHAR(40),
    SMALL_VAL    SMALLINT,
    BIG_VAL      BIGINT,
    DEC_VAL      NUMERIC(15,4),
    FLT_VAL      FLOAT,
    DBL_VAL      DOUBLE PRECISION,
    DT_VAL       DATE,
    TM_VAL       TIME,
    TS_VAL       TIMESTAMP,
    NOTES_BLOB   BLOB SUB_TYPE TEXT,
    BOOL_VAL     BOOLEAN,
    INT128_VAL   INT128,
    NUM38_VAL    NUMERIC(38,6),
    DEC16_VAL    DECFLOAT(16),
    DEC34_VAL    DECFLOAT(34),
    TIMETZ_VAL   TIME WITH TIME ZONE,
    TSTZ_VAL     TIMESTAMP WITH TIME ZONE,
    NULLABLE_COL VARCHAR(20)
);

COMMIT;

-- Populando DEPARTMENT
INSERT INTO DEPARTMENT VALUES ('000', 'Corporate Headquarters', NULL, 2500000.00, 'Sao Paulo');
INSERT INTO DEPARTMENT VALUES ('100', 'Sales and Marketing', '000', 1200000.50, 'Sao Paulo');
INSERT INTO DEPARTMENT VALUES ('600', 'Engineering', '000', 3450000.75, 'Campinas');
INSERT INTO DEPARTMENT VALUES ('621', 'Software Development', '600', 1850000.00, 'Campinas');
INSERT INTO DEPARTMENT VALUES ('622', 'Quality Assurance', '600', 780000.25, 'Curitiba');
INSERT INTO DEPARTMENT VALUES ('623', 'Customer Support', '600', 620000.00, 'Belo Horizonte');

-- Populando JOB
INSERT INTO JOB VALUES ('Eng', 5, 'Brazil', 'Senior Software Engineer', 10000.00, 18000.00, 'Dominio de C/C++, Linux, Firebird 3 OO API, depuracao com GDB/Valgrind e arquitetura de sistemas de alta performance.');
INSERT INTO JOB VALUES ('Eng', 4, 'Brazil', 'Software Engineer', 7000.00, 12000.00, 'Experiencia com desenvolvimento C++, SQL, controle de versao Git e testes automatizados.');
INSERT INTO JOB VALUES ('Mngr', 6, 'Brazil', 'Engineering Manager', 15000.00, 25000.00, 'Lideranca tecnica de equipes de engenharia de software, planejamento de arquitetura e entregas.');
INSERT INTO JOB VALUES ('QA', 3, 'Brazil', 'QA Analyst', 5000.00, 9000.00, 'Criacao de planos de teste, automacao de testes de regressao e validacao de banco de dados.');
INSERT INTO JOB VALUES ('Sales', 4, 'Brazil', 'Account Executive', 6000.00, 14000.00, 'Gestao de contas corporativas, prospeccao e fechamento de contratos B2B.');

-- Populando EMPLOYEE
INSERT INTO EMPLOYEE VALUES (101, 'Mauricio', 'Junqueira', '621', 'Mngr', 6, '2017-12-27', 19500.00);
INSERT INTO EMPLOYEE VALUES (102, 'Ana', 'Beatriz', '621', 'Eng', 5, '2018-03-15', 14800.50);
INSERT INTO EMPLOYEE VALUES (103, 'Roberto', 'Almeida', '621', 'Eng', 4, '2019-07-01', 9800.75);
INSERT INTO EMPLOYEE VALUES (104, 'Fernanda', 'Costa', '622', 'QA', 3, '2020-02-10', 7250.00);
INSERT INTO EMPLOYEE VALUES (105, 'Lucas', 'Mendes', '100', 'Sales', 4, '2021-05-20', 8900.25);
INSERT INTO EMPLOYEE VALUES (106, 'Patricia', 'Rocha', '623', 'Eng', 4, '2022-09-12', 10200.00);
INSERT INTO EMPLOYEE VALUES (107, 'Gustavo', 'Nunes', '600', 'Eng', 5, '2018-11-18', 15600.80);

-- Populando PROJECT
INSERT INTO PROJECT VALUES ('FBCON', 'FBConnect C++ Interface', 101, 'Middleware', 'Biblioteca C++ de integracao direta com as novas interfaces orientadas a objetos (IMaster, IProvider, IAttachment, ITransaction, IStatement, IResultSet, IBlob) do Firebird 3.0+.');
INSERT INTO PROJECT VALUES ('SGERP', 'SoftGo ERP Core', 102, 'ERP', 'Nucleo de processamento transacional de alta concorrencia com suporte a multiplas conexoes, controle estrito de transacoes Read Committed e manipulacao de BLOBs.');
INSERT INTO PROJECT VALUES ('BIAPI', 'Analytics & Telemetry', 107, 'Analytics', 'Pipeline de coleta e agregacao de indicadores operacionais em tempo real integrando Firebird, PostgreSQL, Redis e RabbitMQ.');
INSERT INTO PROJECT VALUES ('QAAUT', 'Suite de Testes Automatizados', 104, 'Tooling', 'Bateria automatizada de testes de regressao e verificacao de vazamento de memoria com Valgrind.');

-- Populando EMPLOYEE_PROJECT
INSERT INTO EMPLOYEE_PROJECT VALUES (101, 'FBCON');
INSERT INTO EMPLOYEE_PROJECT VALUES (102, 'FBCON');
INSERT INTO EMPLOYEE_PROJECT VALUES (102, 'SGERP');
INSERT INTO EMPLOYEE_PROJECT VALUES (103, 'SGERP');
INSERT INTO EMPLOYEE_PROJECT VALUES (104, 'QAAUT');
INSERT INTO EMPLOYEE_PROJECT VALUES (106, 'SGERP');
INSERT INTO EMPLOYEE_PROJECT VALUES (107, 'BIAPI');

-- Populando SALARY_HISTORY
INSERT INTO SALARY_HISTORY VALUES (101, '2019-01-15 10:30:00', 'ADMIN', 16500.00, 18.18);
INSERT INTO SALARY_HISTORY VALUES (102, '2020-06-01 14:15:30', 'MAURICIO', 12500.00, 18.404);
INSERT INTO SALARY_HISTORY VALUES (103, '2021-08-10 09:00:00', 'MAURICIO', 8500.00, 15.3029);
INSERT INTO SALARY_HISTORY VALUES (107, '2022-03-20 16:45:12', 'ADMIN', 13800.00, 13.0492);

-- Populando CUSTOMER
INSERT INTO CUSTOMER VALUES (1001, 'Mautec Tecnologia Ltda', 'Mauricio', 'Junqueira', '+55-11-99999-0001', 'Sao Paulo', 'Brazil');
INSERT INTO CUSTOMER VALUES (1002, 'SoftGo Sistemas S.A.', 'Carlos', 'Eduardo', '+55-19-98888-0002', 'Campinas', 'Brazil');
INSERT INTO CUSTOMER VALUES (1003, 'Horizonte Logistica', 'Mariana', 'Silva', '+55-31-97777-0003', 'Belo Horizonte', 'Brazil');
INSERT INTO CUSTOMER VALUES (1004, 'Sul Cloud Services', 'Rafael', 'Kowalski', '+55-41-96666-0004', 'Curitiba', 'Brazil');
INSERT INTO CUSTOMER VALUES (1005, 'Atlantico Comercio Exterior', 'Helena', 'Santos', '+55-21-95555-0005', 'Rio de Janeiro', 'Brazil');

-- Populando TYPE_SHOWCASE (Firebird 5.x)
INSERT INTO TYPE_SHOWCASE VALUES (
    1,
    'FIX-000001',
    'Registro Completo Positivo',
    32000,
    9876543210123,
    123456.7895,
    3.14159,
    2.718281828459,
    '2026-11-07',
    '14:35:22.1230',
    '2026-11-07 14:35:22.1230',
    'BLOB de texto longo do registro 1 com multiplos segmentos para exercitar leitura no Firebird 5.x.',
    TRUE,
    170141183460469231731687303715884105727,
    99999999999999999999999999999999.123456,
    1.234567890123456E+120,
    3.141592653589793238462643383279502E+1000,
    '14:35:22.1230 America/Sao_Paulo',
    '2026-11-07 14:35:22.1230 America/Sao_Paulo',
    NULL
);

INSERT INTO TYPE_SHOWCASE VALUES (
    2,
    'FIX-000002',
    'Registro Com Valores Negativos',
    -1500,
    -5000000000,
    -42.0050,
    -0.5,
    -99999.123456,
    '2018-01-17',
    '08:05:09.0050',
    '2018-01-17 08:05:09.0050',
    'Segundo BLOB de teste verificando reutilizacao do handler entre linhas consecutivas do cursor.',
    FALSE,
    -170141183460469231731687303715884105727,
    -123456789012345678901234567890.654321,
    -9.876543210987654E-95,
    -2.718281828459045235360287471352662E-500,
    '08:05:09.0050 -03:00',
    '2018-01-17 08:05:09.0050 UTC',
    'Preenchido'
);

COMMIT;


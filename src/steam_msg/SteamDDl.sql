CREATE TABLE
    IF NOT EXISTS steam_users (
        steam_id INT PRIMARY KEY NOT NULL,
        account_name TEXT UNIQUE,
        acces_token TEXT,
        refresh_token TEXT
    );

CREATE TABLE
    IF NOT EXISTS steam_servers (timestamp INTEGER PRIMARY KEY NOT NULL, list TEXT);

CREATE TABLE
    IF NOT EXISTS metadata (version INTEGER);
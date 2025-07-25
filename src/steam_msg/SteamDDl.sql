CREATE TABLE
    IF NOT EXISTS STEAMUSER (
        SteamID INT PRIMARY KEY NOT NULL,
        AccountName TEXT UNIQUE,
        AccessToken TEXT,
        RefreshToken TEXT
    );

CREATE TABLE
    IF NOT EXISTS STEAMSERVER (
        TimeStamp INTEGER  PRIMARY KEY NOT NULL,
        List TEXT
    );
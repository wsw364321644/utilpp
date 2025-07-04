CREATE TABLE
    IF NOT EXISTS STEAMUSER (
        SteamID INT PRIMARY KEY NOT NULL,
        AccountName TEXT UNIQUE,
        AccessToken TEXT,
        RefreshToken TEXT
    );
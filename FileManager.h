#pragma once
#include <QString>
#include <vector>

struct SaveData {
    int stage;
    int distance;
    int coins;
    int durability;
    int stamina;
    int fishCaught;
    int fishTotalValue;
    int gameSeconds;
    bool isDead;
};

struct HighScoreEntry {
    char name[20];
    int score;
    int distance;
    int kills;
    int fishCaught;
    int fishTotalValue;
    int gameSeconds;
    int stagesCleared;
};

struct FishEntry {
    int fishID;
    bool discovered;
    char name[30];
};

class FileManager {
public:
    FileManager();
    void saveGame(const SaveData& data);
    bool loadGame(SaveData& data);
    bool hasSave();
    void deleteSave();
    void markFishDiscovered(int fishID, const char* fishName);
    bool isFishDiscovered(int fishID);
    void saveHighScore(const char* name, int score, int distance,
        int kills, int fishCaught, int fishTotalValue,
        int gameSeconds, int stagesCleared);
    std::vector<HighScoreEntry> loadHighScores();
};

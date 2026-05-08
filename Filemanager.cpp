#define _CRT_SECURE_NO_WARNINGS
#include "FileManager.h"
#include <fstream>
#include <cstring>
#include <algorithm>

FileManager::FileManager()
{
    std::fstream f("Log.dat", std::ios::binary | std::ios::in);
    if (!f.is_open()) {
        std::ofstream init("Log.dat", std::ios::binary);
        FishEntry empty = { 0, false, "" };
        for (int i = 0; i < 10; i++) {
            empty.fishID = i;
            init.write(reinterpret_cast<char*>(&empty), sizeof(FishEntry));
        }
    }
}

void FileManager::saveGame(const SaveData& data)
{
    std::ofstream f("save.dat", std::ios::binary);
    f.write(reinterpret_cast<const char*>(&data), sizeof(SaveData));
}

bool FileManager::loadGame(SaveData& data)
{
    std::ifstream f("save.dat", std::ios::binary);
    if (!f.is_open()) return false;
    f.read(reinterpret_cast<char*>(&data), sizeof(SaveData));
    return true;
}

bool FileManager::hasSave()
{
    std::ifstream f("save.dat", std::ios::binary);
    return f.is_open();
}

void FileManager::deleteSave()
{
    std::remove("save.dat");
}

void FileManager::markFishDiscovered(int fishID, const char* fishName)
{
    std::fstream f("Log.dat", std::ios::binary | std::ios::in | std::ios::out);
    f.seekp(fishID * sizeof(FishEntry));
    FishEntry entry;
    f.read(reinterpret_cast<char*>(&entry), sizeof(FishEntry));
    entry.discovered = true;
    strncpy(entry.name, fishName, 29);
    f.seekp(fishID * sizeof(FishEntry));
    f.write(reinterpret_cast<char*>(&entry), sizeof(FishEntry));
}

bool FileManager::isFishDiscovered(int fishID)
{
    std::ifstream f("Log.dat", std::ios::binary);
    f.seekg(fishID * sizeof(FishEntry));
    FishEntry entry;
    f.read(reinterpret_cast<char*>(&entry), sizeof(FishEntry));
    return entry.discovered;
}

void FileManager::saveHighScore(const char* name, int score, int distance,
    int kills, int fishCaught, int fishTotalValue,
    int gameSeconds, int stagesCleared)
{
    std::vector<HighScoreEntry> scores = loadHighScores();
    HighScoreEntry e;
    strncpy(e.name, name, 19);
    e.score = score; e.distance = distance; e.kills = kills;
    e.fishCaught = fishCaught; e.fishTotalValue = fishTotalValue;
    e.gameSeconds = gameSeconds; e.stagesCleared = stagesCleared;
    scores.push_back(e);
    std::sort(scores.begin(), scores.end(),
        [](const HighScoreEntry& a, const HighScoreEntry& b) { return a.score > b.score; });
    if (scores.size() > 10) scores.resize(10);
    std::ofstream f("highscore.dat", std::ios::binary);
    for (auto& s : scores)
        f.write(reinterpret_cast<char*>(&s), sizeof(HighScoreEntry));
}

std::vector<HighScoreEntry> FileManager::loadHighScores()
{
    std::vector<HighScoreEntry> scores;
    std::ifstream f("highscore.dat", std::ios::binary);
    if (!f.is_open()) return scores;
    HighScoreEntry e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(HighScoreEntry)))
        scores.push_back(e);
    return scores;
}
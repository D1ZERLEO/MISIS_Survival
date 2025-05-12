#include <iostream>
#include <windows.h>
#include <conio.h>
#include <limits>
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

using namespace std;

// Функция для переноса текста
string WrapText(const string& text, size_t line_length = 70) {
    istringstream words(text);
    string line, result;
    string word;

    if (words >> word) {
        line = word;
        while (words >> word) {
            if (line.length() + word.length() + 1 > line_length) {
                result += line + "\n";
                line = word;
            }
            else {
                line += " " + word;
            }
        }
        result += line;
    }

    return result;
}

class ConsoleColor {
public:
    static void SetColor(int color) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, color);
    }
};

class GameState {
public:
    int coins = 1000;
    int reputation = 5;
    int lives = 3;
    int currentLevel = 1;
    int currentMonth = 1;
    bool gameOver = false;

    void AddCoins(int amount) { coins = max(0, coins + amount); }
    void AddReputation(int amount) { reputation = max(min(reputation + amount, 10), 0); }
    void AddLives(int amount) { lives = max(0, lives + amount); }
};

class Question {
public:
    string text;
    vector<string> options;
    vector<string> results;
    vector<vector<int>> effects;

    Question(const string& t, const vector<string>& o, const vector<string>& r, const vector<vector<int>>& e)
        : text(t), options(o), results(r), effects(e) {
    }
};

class QuestionLoader {
private:
    string filename;

    vector<string> SplitString(const string& s, char delimiter) {
        vector<string> tokens;
        string token;
        istringstream tokenStream(s);
        while (getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    vector<vector<int>> ParseEffects(const string& effectsLine) {
        vector<vector<int>> effects;
        vector<string> effectStrings = SplitString(effectsLine, '|');

        for (const string& effectStr : effectStrings) {
            vector<string> parts = SplitString(effectStr, ' ');
            vector<int> effect;
            for (const string& part : parts) {
                if (!part.empty()) {
                    try {
                        effect.push_back(stoi(part));
                    }
                    catch (...) {
                        continue;
                    }
                }
            }
            if (effect.size() >= 2) {
                effects.push_back({ effect[0], effect[1] });
            }
        }

        return effects;
    }

public:
    QuestionLoader(const string& file) : filename(file) {}

    map<int, vector<Question>> LoadQuestions() {
        map<int, vector<Question>> months;
        ifstream file(filename);
        string line;

        if (!file.is_open()) {
            throw runtime_error("Не удалось открыть файл с вопросами: " + filename);
        }

        int currentMonth = 0;
        vector<Question> currentQuestions;
        string currentText;
        vector<string> currentOptions;
        vector<string> currentResults;
        vector<vector<int>> currentEffects;

        while (getline(file, line)) {
            if (line.empty()) continue;

            if (line.find("Месяц") != string::npos) {
                if (currentMonth != 0) {
                    months[currentMonth] = currentQuestions;
                    currentQuestions.clear();
                }
                currentMonth = stoi(line.substr(6));
            }
            else if (line.find("Вопрос") != string::npos) {
                if (!currentText.empty()) {
                    currentQuestions.emplace_back(currentText, currentOptions, currentResults, currentEffects);
                    currentOptions.clear();
                    currentResults.clear();
                    currentEffects.clear();
                }
                currentText = WrapText(line.substr(line.find(":") + 2));
            }
            else if (line.find("Варианты:") != string::npos) {
                currentOptions.clear();
                for (int i = 0; i < 3 && getline(file, line); i++) {
                    size_t dotPos = line.find('.');
                    if (dotPos != string::npos) {
                        currentOptions.push_back(WrapText(line.substr(dotPos + 2)));
                    }
                }
            }
            else if (line.find("Результаты:") != string::npos) {
                currentResults.clear();
                for (int i = 0; i < 3 && getline(file, line); i++) {
                    currentResults.push_back(WrapText(line));
                }
            }
            else if (line.find("Эффекты:") != string::npos) {
                currentEffects = ParseEffects(line.substr(8));
            }
        }

        if (!currentText.empty()) {
            currentQuestions.emplace_back(currentText, currentOptions, currentResults, currentEffects);
        }

        if (currentMonth != 0) {
            months[currentMonth] = currentQuestions;
        }

        file.close();
        return months;
    }
};

class StudentLifeGame {
private:
    GameState state;
    map<int, vector<Question>> months;
    string questionsFile = "../../../../../game/questions.txt";

    void InitializeQuestions() {
        QuestionLoader loader(questionsFile);
        months = loader.LoadQuestions();
    }

    void ProcessChoice(const Question& question) {
        int choice;
        string input;

        while (true) {
            getline(cin, input);
            input.erase(remove(input.begin(), input.end(), ' '), input.end());

            if (input == "1") {
                choice = 1;
            }
            else if (input == "2") {
                choice = 2;
            }
            else if (input == "3") {
                choice = 3;
            }
            else {
                ShowMessage("Ошибка! Введите 1, 2 или 3");
                DrawInterface();
                DrawQuestion(question);
                continue;
            }
            break;
        }

        if (choice >= 1 && choice <= question.effects.size()) {
            const vector<int>& effect = question.effects[choice - 1];
            if (effect.size() >= 2) {
                state.AddCoins(effect[0]);
                state.AddReputation(effect[1]);
            }
        }

        ShowMessage(question.results[choice - 1]);

        state.currentLevel++;
        if (state.currentLevel > months[state.currentMonth].size()) {
            state.currentMonth++;
            state.currentLevel = 1;
        }

        if (state.currentMonth > months.rbegin()->first || state.reputation <= 0 || state.coins <= 0 || state.lives <= 0) {
            state.gameOver = true;
        }
    }

    void ShowMessage(const string& message) {
        system("cls");
        DrawInterface();
        ConsoleColor::SetColor(12);
        cout << "\n" << WrapText(message) << "\n";
        cout << "\nНажмите любую клавишу чтобы продолжить...";
        _getch();
    }

    string GetFixedLengthString(int value, int totalLength) {
        string str = to_string(value);
        while (str.length() < (size_t)totalLength) {
            str = " " + str;
        }
        return str;
    }

public:
    StudentLifeGame() {
        // Установка размера консоли при запуске
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SMALL_RECT rect = { 0, 0, 79, 49 }; // Ширина 80, высота 50
        SetConsoleWindowInfo(hConsole, TRUE, &rect);

        InitializeQuestions();
    }

    void DrawInterface() {
        system("cls");

        ConsoleColor::SetColor(11);
        cout << "----------------------------------------\n";
        cout << "|             MISIS SURVIVAL           |\n";
        cout << "|--------------------------------------|\n";

        ConsoleColor::SetColor(14);
        cout << "| Деньги: " << GetFixedLengthString(state.coins, 4);
        cout << "                         |\n";

        ConsoleColor::SetColor(12);
        cout << "| Репутация: " << GetFixedLengthString(state.reputation, 2) << "/10";
        cout << "                     |\n";

        ConsoleColor::SetColor(10);
        cout << "| Уровень:" << GetFixedLengthString(state.currentLevel, 2);
        cout << " (Месяц " << state.currentMonth << ")                 |\n";

        ConsoleColor::SetColor(13);
        cout << "| Жизни: " << GetFixedLengthString(state.lives, 2);
        cout << "                            |\n";

        ConsoleColor::SetColor(11);
        cout << "|______________________________________|\n";
    }

    void DrawQuestion(const Question& question) {
        ConsoleColor::SetColor(9);
        cout << "| " << WrapText(question.text) << "\n";
        cout << "|                                      \n";

        for (int i = 0; i < question.options.size(); i++) {
            ConsoleColor::SetColor(10 + i);
            cout << "| " << (i + 1) << ". " << WrapText(question.options[i]) << "\n";
        }

        ConsoleColor::SetColor(11);
        cout << "|                                      \n";
        cout << "----------------------------------------\n";
        cout << "> ";
    }

    void ShowGameOver() {
        system("cls");
        if (state.reputation <= 0 || state.coins <= 0 || state.lives <= 0) {
            ConsoleColor::SetColor(12);
            cout << "----------------------------------------\n";
            cout << "|            G A M E   O V E R         |\n";
            cout << "----------------------------------------\n";
        }
        else {
            ConsoleColor::SetColor(10);
            cout << "----------------------------------------\n";
            cout << "|            В Ы   П О Б Е Д И Л И    |\n";
            cout << "----------------------------------------\n";
        }

        ConsoleColor::SetColor(15);
        cout << "\nНажмите любую клавишу для выхода...";
        _getch();
    }

    void Run() {
        SetConsoleOutputCP(1251);
        SetConsoleCP(1251);

        while (!state.gameOver) {
            DrawInterface();

            if (state.currentMonth > months.rbegin()->first ||
                state.currentLevel > months[state.currentMonth].size()) {
                state.gameOver = true;
                break;
            }

            Question currentQuestion = months[state.currentMonth][state.currentLevel - 1];
            DrawQuestion(currentQuestion);

            cin.clear();
            ProcessChoice(currentQuestion);
        }

        ShowGameOver();
    }
};

int main() {
    try {
        StudentLifeGame game;
        game.Run();
    }
    catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
        return 1;
    }
    return 0;
}
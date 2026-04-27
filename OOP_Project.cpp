#include <iostream>
#include <string>
#include <stdexcept>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <thread>
#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
using namespace std;

#define BLUE_BOLD "\033[1;34m"
#define RESET "\033[0m"

#define CROPS_FILE "crops.dat"
#define THRESHOLDS_FILE "thresholds.dat"

#define TOP_MARGIN 3
#define LEFT_MARGIN_SPACES 20

struct MarginBuf : std::streambuf
{
    std::streambuf *dest;
    bool atLineStart;
    std::string margin;
    MarginBuf(std::streambuf *buf, const std::string &m)
        : dest(buf), atLineStart(true), margin(m) {}
    int overflow(int ch) override
    {
        if (atLineStart && ch != '\n')
        {
            dest->sputn(margin.c_str(), margin.size());
            atLineStart = false;
        }
        if (ch == '\n')
        {
            atLineStart = true;
            dest->sputc(ch);
        }
        else
        {
            dest->sputc(ch);
        }
        return ch;
    }
};

void showLoadingAnimation()
{
    cout << BLUE_BOLD;
    for (int i = 0; i < 4; ++i)
    {
        cout << ".";
        cout.flush();
#ifdef _WIN32
        Sleep(300);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
#endif
    }
    cout << RESET << endl;
}

class Date
{
private:
    int day, month, year;

public:
    Date(int d = 1, int m = 1, int y = 2000)
        : day(d), month(m), year(y) {}

    Date(const Date &other)
        : day(other.day), month(other.month), year(other.year) {}

    Date &operator=(const Date &other)
    {
        if (this != &other)
        {
            day = other.day;
            month = other.month;
            year = other.year;
        }
        return *this;
    }

    friend ostream &operator<<(ostream &os, const Date &dt)
    {
        os << setw(2) << setfill('0') << dt.day << "/"
           << setw(2) << setfill('0') << dt.month << "/"
           << dt.year;
        return os;
    }

    int getDay() const { return day; }
    int getMonth() const { return month; }
    int getYear() const { return year; }

    void setDate(int d, int m, int y)
    {
        day = d;
        month = m;
        year = y;
    }
};

class Crop
{
private:
    static int nextId;
    int id;
    string name;
    float moistureThreshold;
    Date plantingDate;
    int growthStage;
    int daysSinceFertilization;

public:
    Crop()
        : id(nextId++),
          name(""),
          moistureThreshold(0.0f),
          plantingDate(),
          growthStage(0),
          daysSinceFertilization(0)
    {
    }

    Crop(const string &nm, float thresh, const Date &pd)
        : id(nextId++),
          name(nm),
          moistureThreshold(thresh),
          plantingDate(pd),
          growthStage(0),
          daysSinceFertilization(0)
    {
    }

    Crop(const Crop &other)
        : id(nextId++),
          name(other.name),
          moistureThreshold(other.moistureThreshold),
          plantingDate(other.plantingDate),
          growthStage(other.growthStage),
          daysSinceFertilization(other.daysSinceFertilization)
    {
    }

    Crop &operator=(const Crop &other)
    {
        if (this != &other)
        {
            name = other.name;
            moistureThreshold = other.moistureThreshold;
            plantingDate = other.plantingDate;
            growthStage = other.growthStage;
            daysSinceFertilization = other.daysSinceFertilization;
        }
        return *this;
    }

    int getId() const { return id; }
    const string &getName() const { return name; }
    float getMoistureThreshold() const { return moistureThreshold; }
    const Date &getPlantingDate() const { return plantingDate; }
    int getGrowthStage() const { return growthStage; }
    int getDaysSinceFertilization() const { return daysSinceFertilization; }

    void setDaysSinceFertilization(int d)
    {
        if (d < 0)
            throw invalid_argument("Days cannot be negative");
        daysSinceFertilization = d;
    }

    void advanceStage()
    {
        if (growthStage < 5)
            ++growthStage;
    }

    void setName(const string &nm) { name = nm; }
    void setMoistureThreshold(float thresh)
    {
        if (thresh < 0 || thresh > 100)
            throw invalid_argument("Invalid threshold");
        moistureThreshold = thresh;
    }
    void setPlantingDate(const Date &pd) { plantingDate = pd; }
    void setId(int newId) { id = newId; }

    static void updateNextId(int id)
    {
        if (id >= nextId)
        {
            nextId = id + 1;
        }
    }
};

int Crop::nextId = 1;

class Sensor
{
public:
    virtual ~Sensor() {}
    virtual float getReading() const = 0;
};

class MoistureSensor : public Sensor
{
public:
    float getReading() const override
    {
        return static_cast<float>(rand() % 101);
    }
};

class TemperatureSensor : public Sensor
{
public:
    float getReading() const override
    {
        return -10.0f + static_cast<float>(rand() % 51);
    }
};

class Action
{
public:
    virtual ~Action() {}
    virtual void execute() const = 0;
};

class WaterAction : public Action
{
    string cropName;

public:
    WaterAction(const string &name) : cropName(name) {}
    void execute() const override
    {
        cout << BLUE_BOLD << "    Suggestion: water " << cropName << RESET << endl;
    }
};

class FertilizeAction : public Action
{
    string cropName;

public:
    FertilizeAction(const string &name) : cropName(name) {}
    void execute() const override
    {
        cout << BLUE_BOLD << "    Suggestion: fertilize " << cropName << RESET << endl;
    }
};

class CropManager
{
    Crop *crops;
    int capacity;
    int count;

    void resize(int newCap)
    {
        Crop *tmp = new Crop[newCap];
        for (int i = 0; i < count; ++i)
            tmp[i] = crops[i];
        delete[] crops;
        crops = tmp;
        capacity = newCap;
    }

    void saveCropsToFile() const
    {
        ofstream file(CROPS_FILE, ios::out);
        if (!file)
        {
            cerr << "Failed to open crops file for writing" << endl;
            return;
        }
        ofstream textFile("crops.txt", ios::out);
        if (!textFile)
        {
            cerr << "Failed to open crops.txt for writing" << endl;
        }

        for (int i = 0; i < count; ++i)
        {
            const Crop &c = crops[i];
            string line =
                to_string(c.getId()) + "|" + c.getName() + "|" + to_string(c.getMoistureThreshold()) + "|" + to_string(c.getPlantingDate().getDay()) + "|" + to_string(c.getPlantingDate().getMonth()) + "|" + to_string(c.getPlantingDate().getYear()) + "|" + to_string(c.getGrowthStage()) + "|" + to_string(c.getDaysSinceFertilization());
            file << line << "\n";
            if (textFile)
                textFile << line << "\n";
        }

        file.close();
        if (textFile)
            textFile.close();
    }

    void saveThresholdsToFile() const
    {
        ofstream file(THRESHOLDS_FILE, ios::out);
        if (!file)
        {
            cerr << "Failed to open thresholds file for writing" << endl;
            return;
        }

        for (int i = 0; i < count; ++i)
        {
            const Crop &c = crops[i];
            file << c.getId() << "|"
                 << c.getName() << "|"
                 << c.getMoistureThreshold() << endl;
        }
        file.close();
    }

public:
    CropManager(int cap = 5)
        : capacity(cap), count(0), crops(new Crop[cap])
    {
        loadCropsFromFile();
    }

    CropManager(const CropManager &o)
        : capacity(o.capacity), count(o.count), crops(new Crop[o.capacity])
    {
        for (int i = 0; i < count; ++i)
            crops[i] = o.crops[i];
    }

    CropManager &operator=(const CropManager &o)
    {
        if (this != &o)
        {
            delete[] crops;
            capacity = o.capacity;
            count = o.count;
            crops = new Crop[capacity];
            for (int i = 0; i < count; ++i)
                crops[i] = o.crops[i];
        }
        return *this;
    }

    ~CropManager()
    {
        delete[] crops;
    }

    void loadCropsFromFile()
    {
        ifstream file(CROPS_FILE);
        if (!file)
        {
            return;
        }

        string line;
        count = 0;

        while (getline(file, line))
        {
            if (count >= capacity)
            {
                resize(capacity * 2);
            }

            stringstream ss(line);
            string token;
            vector<string> tokens;

            while (getline(ss, token, '|'))
            {
                tokens.push_back(token);
            }

            if (tokens.size() == 8)
            {
                int id = stoi(tokens[0]);
                string name = tokens[1];
                float threshold = stof(tokens[2]);
                int day = stoi(tokens[3]);
                int month = stoi(tokens[4]);
                int year = stoi(tokens[5]);
                int daysSinceFert = stoi(tokens[7]);

                Date plantingDate(day, month, year);
                Crop c(name, threshold, plantingDate);
                c.setId(id);
                c.setDaysSinceFertilization(daysSinceFert);
                Crop::updateNextId(id);

                crops[count++] = c;
            }
        }

        file.close();
    }

    void addCrop(const Crop &c)
    {
        if (count >= capacity)
            resize(capacity * 2);
        crops[count++] = c;

        saveCropsToFile();
        saveThresholdsToFile();
    }

    Crop &getCropMutable(int idx)
    {
        if (idx < 0 || idx >= count)
            throw out_of_range("Invalid crop index");
        return crops[idx];
    }

    const Crop &getCrop(int idx) const
    {
        if (idx < 0 || idx >= count)
            throw out_of_range("Invalid crop index");
        return crops[idx];
    }

    int getCount() const { return count; }

    Action *evaluateCrop(int idx, float moisture) const
    {
        const Crop &c = getCrop(idx);
        if (moisture < c.getMoistureThreshold())
        {
            return new WaterAction(c.getName());
        }
        const int INTERVAL = 7;
        if (moisture > c.getMoistureThreshold() * 1.2f && c.getDaysSinceFertilization() > INTERVAL)
        {
            return new FertilizeAction(c.getName());
        }
        return nullptr;
    }

    void listCrops() const
    {
        cout << BLUE_BOLD << "Registered Crops" << RESET << endl;
        cout << BLUE_BOLD << "----------------" << RESET << endl
             << endl;
        for (int i = 0; i < count; ++i)
        {
            const Crop &c = crops[i];
            cout << BLUE_BOLD << "Crop " << (i + 1) << ":" << RESET << endl
                 << BLUE_BOLD << "  Name: " << c.getName() << RESET << endl
                 << BLUE_BOLD << "  Threshold: " << c.getMoistureThreshold() << "%" << RESET << endl
                 << BLUE_BOLD << "  Planted: " << c.getPlantingDate() << RESET << endl
                 << BLUE_BOLD << "  Days since fertilization: "
                 << c.getDaysSinceFertilization() << RESET << endl
                 << endl;
        }
    }

    void updateThreshold(int idx, float newThreshold)
    {
        if (idx < 0 || idx >= count)
            throw out_of_range("Invalid crop index");

        crops[idx].setMoistureThreshold(newThreshold);
        saveThresholdsToFile();
    }

    void updateCropData(int idx)
    {
        if (idx < 0 || idx >= count)
            throw out_of_range("Invalid crop index");

        saveCropsToFile();
        saveThresholdsToFile();
    }

    void deleteCrop(int idx)
    {
        if (idx < 0 || idx >= count)
            throw out_of_range("Invalid crop index");
        for (int i = idx; i < count - 1; ++i)
            crops[i] = crops[i + 1];
        --count;

        saveCropsToFile();
        saveThresholdsToFile();
    }
};

class PrecisionFarmSystem
{
    CropManager cropMgr;
    Sensor **sensors;
    int sensorCount;
    int sensorCapacity;

public:
    PrecisionFarmSystem(int sc = 2)
        : cropMgr(),
          sensorCount(0),
          sensorCapacity(sc),
          sensors(new Sensor *[sc])
    {
    }

    ~PrecisionFarmSystem()
    {
        for (int i = 0; i < sensorCount; ++i)
            delete sensors[i];
        delete[] sensors;
    }

    void addSensor(Sensor *s)
    {
        if (sensorCount >= sensorCapacity)
            throw overflow_error("Too many sensors");
        sensors[sensorCount++] = s;
    }

    void registerCrop(const Crop &c)
    {
        cropMgr.addCrop(c);
    }

    void runCycle()
    {
        if (cropMgr.getCount() == 0)
        {
            cout << BLUE_BOLD << "No crops registered yet. Cannot fulfill request." << RESET << endl;
            return;
        }
        cout << BLUE_BOLD << "=== Monitoring Cycle ===" << RESET << endl
             << endl;
        int n = cropMgr.getCount();
        for (int i = 0; i < n; ++i)
        {
            Crop &cm = cropMgr.getCropMutable(i);
            cm.setDaysSinceFertilization(cm.getDaysSinceFertilization() + 1);

            cout << BLUE_BOLD << "Crop: " << cm.getName() << RESET << endl;

            for (int j = 0; j < sensorCount; ++j)
            {
                MoistureSensor *ms = dynamic_cast<MoistureSensor *>(sensors[j]);
                if (ms)
                {
                    float m = ms->getReading();
                    cout << BLUE_BOLD << "  Moisture: " << m << " %" << RESET << endl;
                    Action *act = cropMgr.evaluateCrop(i, m);
                    if (act)
                    {
                        act->execute();
                        if (dynamic_cast<WaterAction *>(act))
                        {
                            cm.advanceStage();
                            cout << BLUE_BOLD << "    -> Growth stage is now: "
                                 << cm.getGrowthStage() << RESET << endl;
                        }
                        else if (dynamic_cast<FertilizeAction *>(act))
                        {
                            cm.setDaysSinceFertilization(0);
                            cout << BLUE_BOLD << "    -> Days since fertilization reset to 0"
                                 << RESET << endl;
                        }
                        delete act;
                    }
                    continue;
                }

                TemperatureSensor *ts = dynamic_cast<TemperatureSensor *>(sensors[j]);
                if (ts)
                {
                    float t = ts->getReading();
                    cout << BLUE_BOLD << "  Temperature: " << t << " C" << RESET << endl;
                }
            }
            cout << endl;
        }
    }

    void displayCrops() const
    {
        if (cropMgr.getCount() == 0)
        {
            cout << BLUE_BOLD << "No crops registered yet. Cannot fulfill request." << RESET << endl;
            return;
        }
        cropMgr.listCrops();
    }

    void checkThresholds()
    {
        if (cropMgr.getCount() == 0)
        {
            cout << BLUE_BOLD << "No crops registered yet. Cannot fulfill request." << RESET << endl;
            return;
        }
        int n = cropMgr.getCount();
        bool issuesFound = false;
        for (int i = 0; i < n; ++i)
        {
            Crop &cm = cropMgr.getCropMutable(i);
            for (int j = 0; j < sensorCount; ++j)
            {
                MoistureSensor *ms = dynamic_cast<MoistureSensor *>(sensors[j]);
                if (ms)
                {
                    float m = ms->getReading();
                    if (m < cm.getMoistureThreshold())
                    {
                        issuesFound = true;
                        cout << BLUE_BOLD << "WARNING: Crop " << cm.getName()
                             << " moisture (" << m << "%) is below threshold ("
                             << cm.getMoistureThreshold() << "%)" << RESET << endl;
                    }
                }
            }
        }
        if (!issuesFound)
        {
            cout << BLUE_BOLD << "All crops are within threshold levels." << RESET << endl;
            return;
        }
        cout << BLUE_BOLD << "\nSelect crop to update moisture (1-" << n << ", 0 to cancel): " << RESET;
        int idx;
        while (!(cin >> idx))
        {
            cout << BLUE_BOLD << "Invalid input. Enter a number (1-" << n << ", 0 to cancel): " << RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        if (idx == 0)
            return;
        if (idx < 1 || idx > n)
        {
            cout << BLUE_BOLD << "Invalid selection." << RESET << endl;
            return;
        }
        Crop &cm = cropMgr.getCropMutable(idx - 1);
        cout << BLUE_BOLD << "Enter new moisture threshold (0-100): " << RESET;
        float thresh;
        while (!(cin >> thresh))
        {
            cout << BLUE_BOLD << "Invalid input. Enter a number (0-100): " << RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        try
        {
            cm.setMoistureThreshold(thresh);
            cropMgr.updateThreshold(idx - 1, thresh);
            cout << BLUE_BOLD << "Moisture threshold updated successfully." << RESET << endl;
            showLoadingAnimation();
        }
        catch (const invalid_argument &e)
        {
            cout << BLUE_BOLD << "Error: " << e.what() << RESET << endl;
        }
    }

    void updateCrop()
    {
        if (cropMgr.getCount() == 0)
        {
            cout << BLUE_BOLD << "No crops registered yet. Cannot fulfill request." << RESET << endl;
            return;
        }
        displayCrops();
        int n = cropMgr.getCount();
        cout << BLUE_BOLD << "Select crop to update (1-" << n << ", 0 to cancel): " << RESET;
        int idx;
        while (!(cin >> idx))
        {
            cout << BLUE_BOLD << "Invalid input. Enter a number (1-" << n << ", 0 to cancel): " << RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        if (idx == 0)
            return;
        if (idx < 1 || idx > n)
        {
            cout << BLUE_BOLD << "Invalid selection." << RESET << endl;
            return;
        }
        Crop &cm = cropMgr.getCropMutable(idx - 1);
        cout << BLUE_BOLD << "Current details for " << cm.getName() << ":" << RESET << endl;
        cout << BLUE_BOLD << "  Name: " << cm.getName() << RESET << endl;
        cout << BLUE_BOLD << "  Moisture threshold: " << cm.getMoistureThreshold() << "%" << RESET << endl;
        cout << BLUE_BOLD << "  Planted: " << cm.getPlantingDate() << RESET << endl;
        cout << BLUE_BOLD << "  Days since fertilization: " << cm.getDaysSinceFertilization() << RESET << endl;

        bool dataUpdated = false;

        cout << BLUE_BOLD << "\nEnter new name (or press Enter to keep current): " << RESET;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        string name;
        getline(cin, name);
        if (!name.empty())
        {
            cm.setName(name);
            cout << BLUE_BOLD << "Name updated to: " << name << RESET << endl;
            dataUpdated = true;
        }
        else
        {
            cout << BLUE_BOLD << "Name unchanged." << RESET << endl;
        }

        cout << BLUE_BOLD << "Enter new moisture threshold (0-100, or press Enter to keep current): " << RESET;
        string input;
        getline(cin, input);
        if (!input.empty())
        {
            stringstream ss(input);
            float thresh;
            if (ss >> thresh && ss.eof())
            {
                try
                {
                    cm.setMoistureThreshold(thresh);
                    cout << BLUE_BOLD << "Moisture threshold updated to: " << thresh << "%" << RESET << endl;
                    dataUpdated = true;
                }
                catch (const invalid_argument &e)
                {
                    cout << BLUE_BOLD << "Error: " << e.what() << RESET << endl;
                }
            }
            else
            {
                cout << BLUE_BOLD << "Invalid input. Moisture threshold unchanged." << RESET << endl;
            }
        }
        else
        {
            cout << BLUE_BOLD << "Moisture threshold unchanged." << RESET << endl;
        }

        cout << BLUE_BOLD << "Enter new planting date (day month year, or press Enter to keep current): " << RESET;
        getline(cin, input);
        if (!input.empty())
        {
            stringstream ss(input);
            int d, m, y;
            if (ss >> d >> m >> y && ss.eof())
            {
                if (d != 0 || m != 0 || y != 0)
                {
                    cm.setPlantingDate(Date(d, m, y));
                    cout << BLUE_BOLD << "Planting date updated to: " << cm.getPlantingDate() << RESET << endl;
                    dataUpdated = true;
                }
                else
                {
                    cout << BLUE_BOLD << "Planting date unchanged." << RESET << endl;
                }
            }
            else
            {
                cout << BLUE_BOLD << "Invalid input. Planting date unchanged." << RESET << endl;
            }
        }
        else
        {
            cout << BLUE_BOLD << "Planting date unchanged." << RESET << endl;
        }

        cout << BLUE_BOLD << "Enter new days since fertilization (or press Enter to keep current): " << RESET;
        getline(cin, input);
        if (!input.empty())
        {
            stringstream ss(input);
            int days;
            if (ss >> days && ss.eof())
            {
                try
                {
                    cm.setDaysSinceFertilization(days);
                    cout << BLUE_BOLD << "Days since fertilization updated to: " << days << RESET << endl;
                    dataUpdated = true;
                }
                catch (const invalid_argument &e)
                {
                    cout << BLUE_BOLD << "Error: " << e.what() << RESET << endl;
                }
            }
            else
            {
                cout << BLUE_BOLD << "Invalid input. Days since fertilization unchanged." << RESET << endl;
            }
        }
        else
        {
            cout << BLUE_BOLD << "Days since fertilization unchanged." << RESET << endl;
        }

        if (dataUpdated)
        {
            cropMgr.updateCropData(idx - 1);
            cout << BLUE_BOLD << "Crop updated successfully." << RESET << endl;
            showLoadingAnimation();
        }
        else
        {
            cout << BLUE_BOLD << "No changes made to crop." << RESET << endl;
        }
    }

    void deleteCrop()
    {
        if (cropMgr.getCount() == 0)
        {
            cout << BLUE_BOLD << "No crops registered yet. Cannot fulfill request." << RESET << endl;
            return;
        }
        displayCrops();
        int n = cropMgr.getCount();
        cout << BLUE_BOLD << "Select crop to delete (1-" << n << ", 0 to cancel): " << RESET;
        int idx;
        while (!(cin >> idx))
        {
            cout << BLUE_BOLD << "Invalid input. Enter a number (1-" << n << ", 0 to cancel): " << RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        if (idx == 0)
            return;
        if (idx < 1 || idx > n)
        {
            cout << BLUE_BOLD << "Invalid selection." << RESET << endl;
            return;
        }
        try
        {
            cropMgr.deleteCrop(idx - 1);
            cout << BLUE_BOLD << "Crop deleted successfully." << RESET << endl;
            showLoadingAnimation();
        }
        catch (const out_of_range &e)
        {
            cout << BLUE_BOLD << "Error: " << e.what() << RESET << endl;
        }
    }
};

void clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    for (int i = 0; i < TOP_MARGIN; ++i)
        cout << "\n";
}

void displayMenu()
{
    clearScreen();
    cout << BLUE_BOLD;
    cout << "=====================================" << endl;
    cout << " Precision Crop Automation System" << endl;
    cout << "=====================================" << endl;
    cout << "1. Register New Crop" << endl;
    cout << "2. Run Monitoring Cycle" << endl;
    cout << "3. Display All Crops" << endl;
    cout << "4. Check and Update Thresholds" << endl;
    cout << "5. Update Crop Information" << endl;
    cout << "6. Delete Crop" << endl;
    cout << "7. Clear Screen" << endl;
    cout << "8. Exit" << endl;
    cout << "Enter choice (1-8): ";
    cout << RESET;
}

void registerCrop(PrecisionFarmSystem &system)
{
    string name;
    float thresh;
    int d, m, y, days;
    cout << BLUE_BOLD << "\nCrop name: " << RESET;
    cin >> ws;
    getline(cin, name);
    cout << BLUE_BOLD << "Moisture threshold (0-100): " << RESET;
    while (!(cin >> thresh))
    {
        cout << BLUE_BOLD << "Invalid input. Enter a number (0-100): " << RESET;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cout << BLUE_BOLD << "Planting date (day month year): " << RESET;
    while (!(cin >> d >> m >> y))
    {
        cout << BLUE_BOLD << "Invalid input. Enter three numbers (day month year): " << RESET;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cout << BLUE_BOLD << "Days since last fertilization: " << RESET;
    while (!(cin >> days))
    {
        cout << BLUE_BOLD << "Invalid input. Enter a number: " << RESET;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    Crop c(name, thresh, Date(d, m, y));
    try
    {
        c.setDaysSinceFertilization(days);
        system.registerCrop(c);
        cout << BLUE_BOLD << "Crop registered successfully." << RESET << endl;
        showLoadingAnimation();
    }
    catch (const invalid_argument &e)
    {
        cout << BLUE_BOLD << "Error: " << e.what() << RESET << endl;
    }
    cout << BLUE_BOLD << "Press Enter to return to menu..." << RESET;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

int main()
{
    const std::string leftMargin(LEFT_MARGIN_SPACES, ' ');
    static MarginBuf mbuf(std::cout.rdbuf(), leftMargin);
    std::cout.rdbuf(&mbuf);

    PrecisionFarmSystem system;
    system.addSensor(new MoistureSensor());
    system.addSensor(new TemperatureSensor());

    char choice;
    do
    {
        displayMenu();
        cin >> choice;
        if (choice == '7')
        {
            clearScreen();
            continue;
        }
        if (choice < '1' || choice > '8')
        {
            cout << BLUE_BOLD << "Invalid choice. Press Enter to continue..." << RESET;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
            continue;
        }
        switch (choice)
        {
        case '1':
            registerCrop(system);
            break;
        case '2':
            clearScreen();
            system.runCycle();
            cout << BLUE_BOLD << "Press Enter to return to menu..." << RESET;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
            break;
        case '3':
            clearScreen();
            system.displayCrops();
            cout << BLUE_BOLD << "Press Enter to return to menu..." << RESET;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
            break;
        case '4':
            clearScreen();
            system.checkThresholds();
            cout << BLUE_BOLD << "Press Enter to return to menu..." << RESET;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
            break;
        case '5':
            clearScreen();
            system.updateCrop();
            cout << BLUE_BOLD << "Press Enter to return to menu..." << RESET;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
            break;
        case '6':
            clearScreen();
            system.deleteCrop();
            cout << BLUE_BOLD << "Press Enter to return to menu..." << RESET;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
            break;
        case '8':
            cout << BLUE_BOLD << "Shutting down system." << RESET << endl;
            showLoadingAnimation();
            break;
        }
    } while (choice != '8');

    return 0;
}

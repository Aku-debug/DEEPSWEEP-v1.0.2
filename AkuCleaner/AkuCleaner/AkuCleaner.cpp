#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cwchar>
#include <locale>
#include <codecvt>
#include <regex>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Shell32.lib")

using namespace std;

// Uygulama bilgilerini tutan yapı
struct AppInfo {
    wstring name;
    wstring publisher;
    wstring version;
    wstring installLocation;
    wstring uninstallString;
    long long sizeKB; // KB cinsinden boyut
    wstring installDate;
};

// Renkli yazı yazmak için fonksiyon
void SetColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// Başlık yazdırma fonksiyonu
void PrintHeader(const string& title) {
    SetColor(14); // Sarı renk
    cout << "\n=== " << title << " ===" << endl;
    SetColor(7); // Beyaz renk
}

// Hata mesajı yazdırma fonksiyonu
void PrintError(const string& message) {
    SetColor(12); // Kırmızı renk
    cerr << "[HATA] " << message << endl;
    SetColor(7); // Beyaz renk
}

// Başarı mesajı yazdırma fonksiyonu
void PrintSuccess(const string& message) {
    SetColor(10); // Yeşil renk
    cout << "[BASARI] " << message << endl;
    SetColor(7); // Beyaz renk
}

// Uyarı mesajı yazdırma fonksiyonu
void PrintWarning(const string& message) {
    SetColor(14); // Sarı renk
    cout << "[UYARI] " << message << endl;
    SetColor(7); // Beyaz renk
}

// Bilgi mesajı yazdırma fonksiyonu
void PrintInfo(const string& message) {
    SetColor(11); // Açık mavi renk
    cout << "[BILGI] " << message << endl;
    SetColor(7); // Beyaz renk
}

// Boyutu uygun formatta gösteren fonksiyon
wstring FormatSize(long long sizeKB) {
    wstringstream wss;

    if (sizeKB < 1024) {
        wss << sizeKB << L" KB";
    }
    else if (sizeKB < 1024 * 1024) {
        wss << fixed << setprecision(2) << (double)sizeKB / 1024 << L" MB";
    }
    else {
        wss << fixed << setprecision(2) << (double)sizeKB / (1024 * 1024) << L" GB";
    }

    return wss.str();
}

// Klasör boyutunu hesaplayan fonksiyon
long long CalculateFolderSize(const wstring& folderPath) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    long long totalSize = 0;
    wstring searchPath = folderPath + L"\\*";

    hFind = FindFirstFile(searchPath.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }

    do {
        const wstring fileOrDirName = findFileData.cFileName;

        // . ve .. dizinlerini atla
        if (fileOrDirName == L"." || fileOrDirName == L"..") {
            continue;
        }

        const wstring fullPath = folderPath + L"\\" + fileOrDirName;

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Alt dizinlerin boyutunu recursive olarak hesapla
            totalSize += CalculateFolderSize(fullPath);
        }
        else {
            // Dosya boyutunu ekle
            totalSize += (static_cast<long long>(findFileData.nFileSizeHigh) << 32) + findFileData.nFileSizeLow;
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
    return totalSize / 1024; // KB cinsinden döndür
}

// Registry'den uygulama boyutunu alan fonksiyon
long long GetAppSizeFromRegistry(HKEY hKey) {
    DWORD sizeLow = 0, sizeHigh = 0;
    DWORD sizeLowSize = sizeof(DWORD);
    DWORD sizeHighSize = sizeof(DWORD);
    long long sizeKB = 0;

    if (RegQueryValueEx(hKey, L"EstimatedSize", NULL, NULL,
        reinterpret_cast<LPBYTE>(&sizeLow), &sizeLowSize) == ERROR_SUCCESS) {
        sizeKB = sizeLow;
    }

    // Bazı uygulamalar farklı anahtar kullanıyor
    else if (RegQueryValueEx(hKey, L"Size", NULL, NULL,
        reinterpret_cast<LPBYTE>(&sizeLow), &sizeLowSize) == ERROR_SUCCESS) {
        sizeKB = sizeLow;
    }

    return sizeKB;
}

// Uygulama boyutunu hesaplayan fonksiyon
long long GetAppSize(const AppInfo& app) {
    // Önce registry'den boyutu almayı dene
    if (app.sizeKB > 0) {
        return app.sizeKB;
    }

    // Kurulum konumundan boyutu hesapla
    if (!app.installLocation.empty()) {
        long long folderSize = CalculateFolderSize(app.installLocation);
        if (folderSize > 0) {
            return folderSize;
        }
    }

    return 0; // Boyut bilinmiyor
}

// Yüklü uygulamaları listeleme fonksiyonu (WMI ve Registry kombinasyonu)
vector<AppInfo> GetInstalledApps() {
    vector<AppInfo> apps;

    // 1. Önce Registry'den temel bilgileri al
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {

        DWORD index = 0;
        wchar_t subKeyName[255];
        DWORD subKeyNameSize = 255;

        while (RegEnumKeyEx(hKey, index, subKeyName, &subKeyNameSize,
            NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

            HKEY hSubKey;
            wstring subKeyPath = wstring(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + subKeyName;

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKeyPath.c_str(),
                0, KEY_READ | KEY_WOW64_64KEY, &hSubKey) == ERROR_SUCCESS) {

                AppInfo app;
                wchar_t displayName[255];
                DWORD displayNameSize = sizeof(displayName);
                wchar_t publisher[255];
                DWORD publisherSize = sizeof(publisher);
                wchar_t displayVersion[255];
                DWORD displayVersionSize = sizeof(displayVersion);
                wchar_t installLocation[1024];
                DWORD installLocationSize = sizeof(installLocation);
                wchar_t uninstallString[1024];
                DWORD uninstallStringSize = sizeof(uninstallString);
                wchar_t installDate[255];
                DWORD installDateSize = sizeof(installDate);

                // Temel bilgileri al
                if (RegGetValue(hSubKey, NULL, L"DisplayName", RRF_RT_REG_SZ,
                    NULL, displayName, &displayNameSize) == ERROR_SUCCESS) {
                    app.name = displayName;

                    // Diğer bilgileri al
                    if (RegGetValue(hSubKey, NULL, L"Publisher", RRF_RT_REG_SZ,
                        NULL, publisher, &publisherSize) == ERROR_SUCCESS) {
                        app.publisher = publisher;
                    }

                    if (RegGetValue(hSubKey, NULL, L"DisplayVersion", RRF_RT_REG_SZ,
                        NULL, displayVersion, &displayVersionSize) == ERROR_SUCCESS) {
                        app.version = displayVersion;
                    }

                    if (RegGetValue(hSubKey, NULL, L"InstallLocation", RRF_RT_REG_SZ,
                        NULL, installLocation, &installLocationSize) == ERROR_SUCCESS) {
                        app.installLocation = installLocation;
                    }

                    if (RegGetValue(hSubKey, NULL, L"UninstallString", RRF_RT_REG_SZ,
                        NULL, uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
                        app.uninstallString = uninstallString;
                    }

                    if (RegGetValue(hSubKey, NULL, L"InstallDate", RRF_RT_REG_SZ,
                        NULL, installDate, &installDateSize) == ERROR_SUCCESS) {
                        app.installDate = installDate;
                    }

                    // Boyut bilgisini al
                    app.sizeKB = GetAppSizeFromRegistry(hSubKey);

                    // Uygulama listesine ekle
                    apps.push_back(app);
                }

                RegCloseKey(hSubKey);
            }

            index++;
            subKeyNameSize = 255;
        }

        RegCloseKey(hKey);
    }

    // 2. 32-bit uygulamalar için WOW64 anahtarını kontrol et
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        0, KEY_READ | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS) {

        DWORD index = 0;
        wchar_t subKeyName[255];
        DWORD subKeyNameSize = 255;

        while (RegEnumKeyEx(hKey, index, subKeyName, &subKeyNameSize,
            NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

            HKEY hSubKey;
            wstring subKeyPath = wstring(L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + subKeyName;

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKeyPath.c_str(),
                0, KEY_READ | KEY_WOW64_32KEY, &hSubKey) == ERROR_SUCCESS) {

                AppInfo app;
                wchar_t displayName[255];
                DWORD displayNameSize = sizeof(displayName);
                wchar_t publisher[255];
                DWORD publisherSize = sizeof(publisher);
                wchar_t displayVersion[255];
                DWORD displayVersionSize = sizeof(displayVersion);
                wchar_t installLocation[1024];
                DWORD installLocationSize = sizeof(installLocation);
                wchar_t uninstallString[1024];
                DWORD uninstallStringSize = sizeof(uninstallString);
                wchar_t installDate[255];
                DWORD installDateSize = sizeof(installDate);

                // Temel bilgileri al
                if (RegGetValue(hSubKey, NULL, L"DisplayName", RRF_RT_REG_SZ,
                    NULL, displayName, &displayNameSize) == ERROR_SUCCESS) {
                    app.name = displayName;

                    // Diğer bilgileri al
                    if (RegGetValue(hSubKey, NULL, L"Publisher", RRF_RT_REG_SZ,
                        NULL, publisher, &publisherSize) == ERROR_SUCCESS) {
                        app.publisher = publisher;
                    }

                    if (RegGetValue(hSubKey, NULL, L"DisplayVersion", RRF_RT_REG_SZ,
                        NULL, displayVersion, &displayVersionSize) == ERROR_SUCCESS) {
                        app.version = displayVersion;
                    }

                    if (RegGetValue(hSubKey, NULL, L"InstallLocation", RRF_RT_REG_SZ,
                        NULL, installLocation, &installLocationSize) == ERROR_SUCCESS) {
                        app.installLocation = installLocation;
                    }

                    if (RegGetValue(hSubKey, NULL, L"UninstallString", RRF_RT_REG_SZ,
                        NULL, uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
                        app.uninstallString = uninstallString;
                    }

                    if (RegGetValue(hSubKey, NULL, L"InstallDate", RRF_RT_REG_SZ,
                        NULL, installDate, &installDateSize) == ERROR_SUCCESS) {
                        app.installDate = installDate;
                    }

                    // Boyut bilgisini al
                    app.sizeKB = GetAppSizeFromRegistry(hSubKey);

                    // Uygulama listesine ekle (daha önce eklenmediyse)
                    bool exists = false;
                    for (const auto& existingApp : apps) {
                        if (existingApp.name == app.name && existingApp.publisher == app.publisher) {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists) {
                        apps.push_back(app);
                    }
                }

                RegCloseKey(hSubKey);
            }

            index++;
            subKeyNameSize = 255;
        }

        RegCloseKey(hKey);
    }

    // 3. Kullanıcıya özel yüklü uygulamaları kontrol et
    HKEY hUserKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        0, KEY_READ, &hUserKey) == ERROR_SUCCESS) {

        DWORD index = 0;
        wchar_t subKeyName[255];
        DWORD subKeyNameSize = 255;

        while (RegEnumKeyEx(hUserKey, index, subKeyName, &subKeyNameSize,
            NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

            HKEY hSubKey;
            wstring subKeyPath = wstring(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + subKeyName;

            if (RegOpenKeyEx(HKEY_CURRENT_USER, subKeyPath.c_str(),
                0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {

                AppInfo app;
                wchar_t displayName[255];
                DWORD displayNameSize = sizeof(displayName);
                wchar_t publisher[255];
                DWORD publisherSize = sizeof(publisher);
                wchar_t displayVersion[255];
                DWORD displayVersionSize = sizeof(displayVersion);
                wchar_t installLocation[1024];
                DWORD installLocationSize = sizeof(installLocation);
                wchar_t uninstallString[1024];
                DWORD uninstallStringSize = sizeof(uninstallString);
                wchar_t installDate[255];
                DWORD installDateSize = sizeof(installDate);

                // Temel bilgileri al
                if (RegGetValue(hSubKey, NULL, L"DisplayName", RRF_RT_REG_SZ,
                    NULL, displayName, &displayNameSize) == ERROR_SUCCESS) {
                    app.name = displayName;

                    // Diğer bilgileri al
                    if (RegGetValue(hSubKey, NULL, L"Publisher", RRF_RT_REG_SZ,
                        NULL, publisher, &publisherSize) == ERROR_SUCCESS) {
                        app.publisher = publisher;
                    }

                    if (RegGetValue(hSubKey, NULL, L"DisplayVersion", RRF_RT_REG_SZ,
                        NULL, displayVersion, &displayVersionSize) == ERROR_SUCCESS) {
                        app.version = displayVersion;
                    }

                    if (RegGetValue(hSubKey, NULL, L"InstallLocation", RRF_RT_REG_SZ,
                        NULL, installLocation, &installLocationSize) == ERROR_SUCCESS) {
                        app.installLocation = installLocation;
                    }

                    if (RegGetValue(hSubKey, NULL, L"UninstallString", RRF_RT_REG_SZ,
                        NULL, uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
                        app.uninstallString = uninstallString;
                    }

                    if (RegGetValue(hSubKey, NULL, L"InstallDate", RRF_RT_REG_SZ,
                        NULL, installDate, &installDateSize) == ERROR_SUCCESS) {
                        app.installDate = installDate;
                    }

                    // Boyut bilgisini al
                    app.sizeKB = GetAppSizeFromRegistry(hSubKey);

                    // Uygulama listesine ekle (daha önce eklenmediyse)
                    bool exists = false;
                    for (const auto& existingApp : apps) {
                        if (existingApp.name == app.name && existingApp.publisher == app.publisher) {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists) {
                        apps.push_back(app);
                    }
                }

                RegCloseKey(hSubKey);
            }

            index++;
            subKeyNameSize = 255;
        }

        RegCloseKey(hUserKey);
    }

    // Uygulama boyutlarını hesapla (registry'de boyut bilgisi yoksa)
    for (auto& app : apps) {
        if (app.sizeKB == 0 && !app.installLocation.empty()) {
            app.sizeKB = CalculateFolderSize(app.installLocation);
        }
    }

    return apps;
}

// Uygulamayı kaldırma fonksiyonu
bool UninstallApp(const AppInfo& app) {
    wstring command = app.uninstallString;

    // Eğer uninstall string MsiExec.exe içeriyorsa /quiet parametresi ekle
    if (command.find(L"MsiExec.exe") != wstring::npos) {
        if (command.find(L"/quiet") == wstring::npos && command.find(L"/qn") == wstring::npos) {
            command += L" /quiet";
        }
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    wchar_t* cmd = new wchar_t[command.size() + 1];
    wcscpy_s(cmd, command.size() + 1, command.c_str());

    if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        delete[] cmd;
        return false;
    }

    delete[] cmd;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

// Kaldırma sonrası temizlik yapan fonksiyon
void CleanupAfterUninstall(const AppInfo& app) {
    // 1. Kurulum konumunu sil
    if (!app.installLocation.empty()) {
        PrintInfo("Kurulum konumu temizleniyor: " + wstring_convert<codecvt_utf8<wchar_t>>().to_bytes(app.installLocation));

        wstring delCmd = L"cmd /c rmdir /s /q \"" + app.installLocation + L"\"";
        _wsystem(delCmd.c_str());
    }

    // 2. AppData'deki kalıntıları temizle
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        wstring appDataDir = wstring(appDataPath) + L"\\" + app.publisher + L"\\" + app.name;
        if (GetFileAttributesW(appDataDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
            PrintInfo("AppData klasoru temizleniyor: " + wstring_convert<codecvt_utf8<wchar_t>>().to_bytes(appDataDir));

            wstring delCmd = L"cmd /c rmdir /s /q \"" + appDataDir + L"\"";
            _wsystem(delCmd.c_str());
        }
    }

    // 3. LocalAppData'deki kalıntıları temizle
    wchar_t localAppDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath))) {
        wstring localAppDataDir = wstring(localAppDataPath) + L"\\" + app.publisher + L"\\" + app.name;
        if (GetFileAttributesW(localAppDataDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
            PrintInfo("LocalAppData klasoru temizleniyor: " + wstring_convert<codecvt_utf8<wchar_t>>().to_bytes(localAppDataDir));

            wstring delCmd = L"cmd /c rmdir /s /q \"" + localAppDataDir + L"\"";
            _wsystem(delCmd.c_str());
        }
    }

    // 4. ProgramData'daki kalıntıları temizle
    wchar_t programDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, programDataPath))) {
        wstring programDataDir = wstring(programDataPath) + L"\\" + app.publisher + L"\\" + app.name;
        if (GetFileAttributesW(programDataDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
            PrintInfo("ProgramData klasoru temizleniyor: " + wstring_convert<codecvt_utf8<wchar_t>>().to_bytes(programDataDir));

            wstring delCmd = L"cmd /c rmdir /s /q \"" + programDataDir + L"\"";
            _wsystem(delCmd.c_str());
        }
    }

    PrintSuccess("Temizleme islemi tamamlandi.");
}

// Uygulamaları isme göre arama fonksiyonu
vector<AppInfo> SearchApps(const vector<AppInfo>& apps, const wstring& searchTerm) {
    vector<AppInfo> results;
    wstring lowerSearchTerm = searchTerm;
    transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), lowerSearchTerm.begin(), towlower);

    for (const auto& app : apps) {
        wstring lowerAppName = app.name;
        transform(lowerAppName.begin(), lowerAppName.end(), lowerAppName.begin(), towlower);

        if (lowerAppName.find(lowerSearchTerm) != wstring::npos) {
            results.push_back(app);
        }
    }

    return results;
}

// Uygulamaları boyuta göre sıralama fonksiyonu
void SortAppsBySize(vector<AppInfo>& apps, bool ascending = true) {
    sort(apps.begin(), apps.end(), [ascending](const AppInfo& a, const AppInfo& b) {
        return ascending ? (a.sizeKB < b.sizeKB) : (a.sizeKB > b.sizeKB);
        });
}

// Uygulamaları isme göre sıralama fonksiyonu
void SortAppsByName(vector<AppInfo>& apps, bool ascending = true) {
    sort(apps.begin(), apps.end(), [ascending](const AppInfo& a, const AppInfo& b) {
        return ascending ? (a.name < b.name) : (a.name > b.name);
        });
}

// Ana fonksiyon
int main() {
    // Konsol kod sayfasını UTF-8 olarak ayarla
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitle(L"Gelismis Uygulama Kaldirici");

    PrintHeader("Yuklu Uygulamalar Taranıyor...");
    vector<AppInfo> apps = GetInstalledApps();

    if (apps.empty()) {
        PrintError("Hic uygulama bulunamadi veya tarama basarisiz oldu.");
        system("pause");
        return 1;
    }

    // Uygulamaları isme göre sırala
    SortAppsByName(apps);

    while (true) {
        system("cls");
        PrintHeader("Ana Menu");
        cout << "1. Tum Uygulamalari Listele\n";
        cout << "2. Uygulama Ara\n";
        cout << "3. Uygulamalari Boyuta Gore Sirala\n";
        cout << "4. Cikis\n";
        cout << "Seciminiz: ";

        int mainChoice;
        cin >> mainChoice;

        vector<AppInfo> currentList = apps;

        if (mainChoice == 1) {
            currentList = apps;
        }
        else if (mainChoice == 2) {
            wstring searchTerm;
            wcin.ignore();
            PrintHeader("Uygulama Ara");
            cout << "Aranacak uygulama adi: ";
            getline(wcin, searchTerm);
            currentList = SearchApps(apps, searchTerm);
        }
        else if (mainChoice == 3) {
            PrintHeader("Siralama Secenekleri");
            cout << "1. Kucukten Buyuge\n";
            cout << "2. Buyukten Kucuge\n";
            cout << "Seciminiz: ";
            int sortChoice;
            cin >> sortChoice;

            if (sortChoice == 1) SortAppsBySize(currentList, true);
            else if (sortChoice == 2) SortAppsBySize(currentList, false);
            else PrintError("Gecersiz secim!");
        }
        else if (mainChoice == 4) {
            break;
        }
        else {
            PrintError("Gecersiz secim!");
            system("pause");
            continue;
        }

        // Uygulama listesini göster
        bool listShown = false;
        while (true) {
            system("cls");
            PrintHeader("Uygulamalar Listesi");
            SetColor(11); // Açık mavi renk
            wprintf(L"%-4s %-40s %-20s %-15s %-10s %-12s\n", L"No", L"Uygulama Adi", L"Yayinci", L"Surum", L"Boyut", L"Kurulum Tarihi");
            SetColor(7); // Beyaz renk

            for (size_t i = 0; i < currentList.size(); i++) {
                wstring sizeStr = FormatSize(GetAppSize(currentList[i]));
                wprintf(L"%-4d %-40s %-20s %-15s %-10s %-12s\n",
                    i + 1,
                    currentList[i].name.substr(0, 40).c_str(),
                    currentList[i].publisher.substr(0, 20).c_str(),
                    currentList[i].version.substr(0, 15).c_str(),
                    sizeStr.c_str(),
                    currentList[i].installDate.substr(0, 12).c_str());
            }

            listShown = true;

            PrintHeader("Islem Secenekleri");
            cout << "1. Uygulama Kaldir\n";
            cout << "2. Listeyi Yenile\n";
            cout << "3. Ana Menuye Don\n";
            cout << "Seciminiz: ";

            int listChoice;
            cin >> listChoice;

            if (listChoice == 1) {
                PrintHeader("Kaldirma Islemi");
                cout << "Kaldirmak istediginiz uygulamanin numarasini girin: ";
                int choice;
                cin >> choice;

                if (choice < 1 || choice > static_cast<int>(currentList.size())) {
                    PrintError("Gecersiz secim!");
                    system("pause");
                    continue;
                }

                AppInfo selectedApp = currentList[choice - 1];
                wcout << L"\nSecilen Uygulama: " << selectedApp.name << endl;
                wcout << L"Yayinci: " << selectedApp.publisher << endl;
                wcout << L"Surum: " << selectedApp.version << endl;
                wcout << L"Boyut: " << FormatSize(GetAppSize(selectedApp)) << endl;
                wcout << L"Kurulum Tarihi: " << selectedApp.installDate << endl;

                PrintWarning("Bu uygulamayi kaldirmak istediginizden emin misiniz? (Y/N): ");
                char confirm;
                cin >> confirm;

                if (toupper(confirm) == 'Y') {
                    wcout << L"\nKaldirma islemi baslatiliyor: " << selectedApp.name << endl;

                    if (UninstallApp(selectedApp)) {
                        PrintSuccess("Kaldirma komutu basariyla calistirildi.");
                        PrintWarning("Kaldirma isleminin tamamlanmasi biraz zaman alabilir.");

                        // Temizlik yap
                        PrintHeader("Kaldirma Sonrasi Temizlik");
                        CleanupAfterUninstall(selectedApp);

                        // Uygulamayı listeden kaldır
                        apps.erase(remove_if(apps.begin(), apps.end(),
                            [&selectedApp](const AppInfo& app) {
                                return app.name == selectedApp.name &&
                                    app.publisher == selectedApp.publisher;
                            }), apps.end());

                        // Geçerli listeden de kaldır
                        currentList.erase(currentList.begin() + (choice - 1));
                    }
                    else {
                        PrintError("Kaldirma islemi baslatilamadi.");
                        wcout << L"Komut: " << selectedApp.uninstallString << endl;
                    }
                }
                else {
                    PrintWarning("Kaldirma islemi iptal edildi.");
                }

                system("pause");
            }
            else if (listChoice == 2) {
                continue; // Listeyi yenile
            }
            else if (listChoice == 3) {
                break; // Ana menüye dön
            }
            else {
                PrintError("Gecersiz secim!");
                system("pause");
            }
        }
    }

    PrintHeader("Program Sonlandiriliyor");
    return 0;
}
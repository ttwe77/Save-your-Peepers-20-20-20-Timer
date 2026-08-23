taskkill /f /im SaveYourPeepersCPP.exe
g++ -std=c++17 -o SaveYourPeepersCPP.exe SaveYourPeepersCPP.cpp -luser32 -lgdi32 -lcomctl32 -lshell32 -lole32 -ladvapi32 -ldwmapi -static -mwindows -O2
SaveYourPeepersCPP.exe

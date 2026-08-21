taskkill /f /im Overlay.exe
g++ -std=c++17 -mwindows Overlay.cpp -o Overlay.exe -lgdi32 -luser32 -lkernel32 -lwinmm -static -municode -mwindows -O2
Overlay.exe
#include <iostream>
#include <ctime>
#include <conio.h>
#include <windows.h>

#define width 40  // anchura
#define height 20 // altura

using namespace std;

void gotoxy(int x, int y)
{
    HANDLE hcon = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD dwPos;
    dwPos.X = x;
    dwPos.Y = y;
    SetConsoleCursorPosition(hcon, dwPos);
}

void setconsolecolor(WORD c)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void ocultarcursor()
{
    HANDLE hcon = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    cci.dwSize = 1;
    cci.bVisible = FALSE;
    SetConsoleCursorInfo(hcon, &cci); // Llama a la API para aplicar la ocultación del cursor
}

int main()
{
    ocultarcursor();

    bool game = true; // Variable de control declarada e inicializada

    float fps = 15.0f;
    float dt = 1.0f / fps;
    float accumulator = 0.0f;

    clock_t start = clock();

    while (game)
    {
        clock_t end = clock();
        accumulator += float(end - start) / CLOCKS_PER_SEC;
        start = end;

        // Limita el acumulador a 0.2f máximo para evitar descompensaciones de tiempo
        if (accumulator >= 0.2f) accumulator = 0.2f;

        while (accumulator >= dt)
        {
            if (GetKeyState(VK_ESCAPE) & 0x8000) game = !game;

            accumulator -= dt;
        }
    }

    return 0;
}
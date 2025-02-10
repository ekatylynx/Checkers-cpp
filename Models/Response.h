#pragma once

// enum class
// возвращение response для какого либо события в игре
enum class Response
{   
    OK, // успех действия
    BACK, // возвращение на ход назад
    REPLAY, // перезапуск игры
    QUIT, // выход из игры
    CELL// клетка на игровой доске
};

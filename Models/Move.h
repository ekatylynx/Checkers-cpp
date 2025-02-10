#pragma once
#include <stdlib.h>

typedef int8_t POS_T;

struct move_pos
{
    POS_T x, y;             // from // Координаты начальной позиции (откуда)
    POS_T x2, y2;           // to // Координаты конечной позиции (куда)
    POS_T xb = -1, yb = -1; // beaten // Координаты побитой фигуры (если есть)

    // Конструктор для инициализации начальной и конечной позиции
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    // Конструктор для инициализации начальной, конечной позиции и побитой фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Перегрузка оператора == для сравнения двух объектов move_pos
    bool operator==(const move_pos &other) const
    {
        // Сравнивает начальные и конечные координаты двух объектов move_pos
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    // Перегрузка оператора != для сравнения двух объектов move_pos
    bool operator!=(const move_pos &other) const
    {
        return !(*this == other); // Возвращает true, если объекты move_pos не равны
    }
};

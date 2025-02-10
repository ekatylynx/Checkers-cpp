#pragma once
#include <tuple>
#include <SDL.h>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
// Класс Hand обрабатывает события ввода от пользователя
class Hand
{
  public:
    Hand(Board *board) : board(board)
    { // Конструктор класса Hand, инициализирует указатель на объект Board
    }
    // тип POS_T определен в моделях и отвечает за поле и за его координаты
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent; // Объект для хранения событий SDL
        Response resp = Response::OK; // Инициализирует ответ как OK
        int x = -1, y = -1; // Координаты клика мыши
        int xc = -1, yc = -1; // Координаты клетки на доске

        // Бесконечный цикл для обработки событий
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Ожидает события от SDL
            {
                switch (windowEvent.type) // Обрабатывает различные типы событий
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Если окно закрывается, устанавливает ответ как QUIT
                    break;
                case SDL_MOUSEBUTTONDOWN: // Обрабатывает нажатие кнопки мыши
                    x = windowEvent.motion.x; // Получает координаты клика x,y
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1); // Преобразует координаты в индексы клетки
                    yc = int(x / (board->W / 10) - 1);

                    // Определяет ответ на основе координат клетки
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; // Если клетка вне доски и есть история, устанавливает ответ как BACK
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; // Если клетка вне доски, устанавливает ответ как REPLAY
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; // Если клетка на доске, устанавливает ответ как CELL
                    }
                    else
                    {
                        xc = -1; // Сбрасывает координаты, если клетка некорректна
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT: // Обрабатывает события окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size(); // Сбрасывает размер окна, если он изменился
                        break;
                    }
                }
                if (resp != Response::OK) // Если ответ не OK, выходит из цикла
                    break;
            }
        }
        return {resp, xc, yc}; // Возвращает ответ и координаты клетки x,y
    }

    // Метод для ожидания события от пользователя
    Response wait() const
    {
        SDL_Event windowEvent; // Объект для хранения событий SDL
        Response resp = Response::OK; // Инициализирует ответ как OK

        // Бесконечный цикл для обработки событий
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Ожидает события от SDL
            {
                switch (windowEvent.type) // Обрабатывает различные типы событий
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Если окно закрывается, устанавливает ответ как QUIT
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED: // Сбрасывает размер окна, если он изменился
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: {  // Обрабатывает нажатие кнопки мыши
                    int x = windowEvent.motion.x; // Получает координаты клика x,y
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1); // Преобразует координаты в индексы клетки
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY; // Если клетка вне доски, устанавливает ответ как REPLAY
                }
                break;
                }
                if (resp != Response::OK) // Если ответ не OK, выходит из цикла
                    break;
            }
        }
        return resp; // Возвращает ответ
    }
    // Указатель на объект Board для взаимодействия с доской
  private:
    Board *board; 
};

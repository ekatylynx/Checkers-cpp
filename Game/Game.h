#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
      // Конструктор класса со списком инициализаторов до входа в тело конструктора
      // board — это объект, который инициализируется с помощью функции config. config возвращает значения конфигурации для ширины и высоты окна.
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {   
        // таймер для установления начала работы программы
        auto start = chrono::steady_clock::now();
        // Если is_replay истинно, это означает, что игра перезапускается. И в этом случае
        if (is_replay) {   
            // logic инициализируется с новыми параметрами.
            logic = Logic(&board, &config);
            // Конфигурация перезагружается с помощью config.reload().
            config.reload();
            // Игровая доска перерисовывается с помощью board.redraw().
            board.redraw();
        }
        else {
            // Если is_replay ложно, начинается новая игра с помощью board.start_draw().
            board.start_draw();
        }
        is_replay = false;

        // Инициализирует счетчик ходов.
        int turn_num = -1;
        // Для отслеживания, хочет ли игрок выйти из игры.
        bool is_quit = false;
        // Получает максимальное количество ходов из конфигурации.
        const int Max_turns = config("Game", "MaxNumTurns");
        // Игровой цикл продолжается, пока количество ходов меньше максимального. // Кол-во ходов определяется в конфиг файле
        while (++turn_num < Max_turns) {
            // Сброс серии ходов
            beat_series = 0;
            // Возможные ходы для игрока
            logic.find_turns(turn_num % 2);

            // Если нет возможных ходов, прерывает цикл
            if (logic.turns.empty())
                break;

            // Устанавливает максимальную глубину для бота на основе игрока
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));

            // Если текущий игрок не является ботом
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Ожидает ответа игрока
                auto resp = player_turn(turn_num % 2);

                // Если игрок выбирает выйти из игры
                if (resp == Response::QUIT)
                {
                    is_quit = true;
                    break;
                }

                // Если игрок выбирает перезапустить игру
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;
                    break;
                }

                // Если игрок выбирает вернуться на ход назад
                else if (resp == Response::BACK)
                {
                    // Если противник бот, то можно откатить ход
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }

                    // Если нет серии ходов, уменьшает счетчик ходов
                    if (!beat_series)
                        --turn_num;

                    // Откат хода
                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                // Если текущий игрок — бот, выполняет ход бота
                bot_turn(turn_num % 2);
        }
        // таймер для установления конца работы программы
        auto end = chrono::steady_clock::now();

        // Открывает файл для записи времени игры
        ofstream fout(project_path + "log.txt", ios_base::app);

        // Записывает время игры в файл
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        // Если игра перезапускается, рекурсивно вызывает play()
        if (is_replay)
            return play();

        // Если игрок выбрал выйти, возвращает 0
        if (is_quit)
            return 0;

        // Определение результатов игры
        int res = 2;
        if (turn_num == Max_turns)
        {
            res = 0; // Ничья, если достигнуто максимальное количество ходов
        }
        else if (turn_num % 2)
        {
            res = 1; // Победа черных, если последний ход был черным
        }

        // Показывает финальное состояние доски
        board.show_final(res);

        // Ожидает ответа игрока
        auto resp = hand.wait();

        // Если игрок выбирает перезапустить игру, рекурсивно вызывает play()
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }


        return res;
    }

  private:
    void bot_turn(const bool color)
    {
        // Устанавливает таймер для установления начала хода бота
        auto start = chrono::steady_clock::now();

        // Получает задержку в миллисекундах для хода бота из конфигурации
        auto delay_ms = config("Bot", "BotDelayMS");

        // Создает новый поток для обеспечения равномерной задержки на каждом ходу
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms);

        // Находит лучшие возможные ходы для бота заданного цвета (белые или черные)
        auto turns = logic.find_best_turns(color);

        // Ожидает завершения потока задержки
        th.join();

        // Флаг для отслеживания первого хода в серии
        bool is_first = true;
        // making moves

        // Выполнение ходов в цикле
        for (auto turn : turns)
        {
            if (!is_first)
            {
                // Если это не первый ход, добавляет задержку
                SDL_Delay(delay_ms);
            }
            is_first = false; // Сбрасывает флаг после первого хода
            beat_series += (turn.xb != -1); // Увеличивает счетчик серии битых фигур, если ход включает битую фигуру
            board.move_piece(turn, beat_series); // Перемещает фигуру на доске
        }

        // Устанавливает таймер для установления конца хода бота
        auto end = chrono::steady_clock::now();

        // Открывает файл для записи времени хода бота
        ofstream fout(project_path + "log.txt", ios_base::app);

        // Записывает время хода бота в файл лога
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";

        // Закрывает файл лога
        fout.close();
    }

    Response player_turn(const bool color) // Узнаем за кого игрок играет за белые или за черные шашки
    {   
        // Подсвечивает все возможные клетки для хода зеленым цветом
        // return 1 if quit
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells);
        move_pos pos = {-1, -1, -1, -1};
        POS_T x = -1, y = -1;
        // trying to make first move
        // бесконечный цикл, отвечающий за обработку кликов
        // обрабатываем клики до тех пор, пока не сделаем ход
        while (true)
        {   
            // ожидает какой-то клик мышкой // функция get_cell распознает какое именно было нажатие
            auto resp = hand.get_cell();
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp);
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)}; // получение координат нажатой клетки

            bool is_correct = false; // для проверки корректности хода
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true; // Если клетка является возможным ходом, устанавливает флаг в true
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second})
                {
                    pos = turn; // сохранение позиции хода
                    break;
                }
            }
            if (pos.x != -1)
                break; // eсли ход корректен - выход из цикла
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active(); // Снимает активную клетку
                    board.clear_highlight(); // Убирает подсветку клеток
                    board.highlight_cells(cells); // Подсвечивает возможные ходы
                }
                x = -1; // Сбрасывает координаты x,y
                y = -1;
                continue; // Продолжает цикл
            }
            x = cell.first; // Устанавливает координаты x,y
            y = cell.second;
            board.clear_highlight(); // Убирает подсветку клеток
            board.set_active(x, y); // Устанавливает активную клетку

            vector<pair<POS_T, POS_T>> cells2; // Вектор для хранения возможных ходов из выбранной клетки
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2); // Добавляет возможные ходы из выбранной клетки
                }
            }
            board.highlight_cells(cells2); // Подсвечивает возможные ходы из выбранной клетки
        }
        board.clear_highlight(); // Убирает подсветку клеток
        board.clear_active(); // Снимает активную клетку
        board.move_piece(pos, pos.xb != -1); // Перемещает фигуру на доске
        if (pos.xb == -1)
            return Response::OK; // Если ход завершен, возвращает OK

        // continue beating while can
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2); // Находит возможные ходы для продолжения боя
            if (!logic.have_beats)
                break; // Если боев больше нет, выходит из цикла

            vector<pair<POS_T, POS_T>> cells; // Вектор для хранения возможных ходов (координат)
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2); // Добавляет возможные ходы
            }
            board.highlight_cells(cells); // Подсвечивает возможные ходы
            board.set_active(pos.x2, pos.y2); // Устанавливает активную клетку

            // trying to make move
            // Бесконечный цикл для обработки хода в серии боев
            while (true)
            {
                auto resp = hand.get_cell(); // Ожидает нажатия клетки мышью
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp); // Если нажатие не на клетку, возвращает соответствующий ответ
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)}; // Получает координаты нажатой клетки

                bool is_correct = false; // Флаг для проверки корректности хода
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true; // Если клетка является возможным ходом, устанавливает флаг в true
                        pos = turn; // Сохраняет позицию хода
                        break;
                    }
                }
                if (!is_correct)
                    continue; // Если ход некорректен, продолжает цикл

                board.clear_highlight(); // Убирает подсветку клеток
                board.clear_active(); // Снимает активную клетку
                beat_series += 1; // Увеличивает счетчик серии боев
                board.move_piece(pos, beat_series); // Перемещает фигуру на доске
                break; // Выходит из цикла
            }
        }

        return Response::OK; // Возвращает OK, если ход завершен
    }

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};

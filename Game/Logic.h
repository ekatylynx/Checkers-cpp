#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

// mtx - матрица
// color - цвет шашек
// depth - глубина

const int INF = 1e9;

class Logic
{
public:
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        // Инициализирует генератор случайных чисел. Если случайность отключена, использует фиксированное начальное значение (из конфиг файла)
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);

        // Устанавливает режим оценки для бота (например, "NumberAndPotential") (из конфиг файла)
        scoring_mode = (*config)("Bot", "BotScoringType");

        // Устанавливает уровень оптимизации для алгоритма поиска ходов (например, "O1") (из конфиг файла)
        optimization = (*config)("Bot", "Optimization");
    }

    // функция для поиска лучшего хода, начиная с текущего состояния доски
    vector<move_pos> find_best_turns(const bool color)
    {   
        // очистка векторов, которые хранят лучшие ходы и состояния
        next_best_state.clear();
        next_move.clear();

        // Получение текущего состояние игровой доски
        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        // инициализирует текущее состояние
        int cur_state = 0;
        vector<move_pos> res;
        // Цикл восстанавливает последовательность лучших ходов, добавляя их в вектор res
        do
        {
            res.push_back(next_move[cur_state]);
            cur_state = next_best_state[cur_state];
        } while (cur_state != -1 && next_move[cur_state].x != -1);
        // возвращает вектор res, содержащий последовательность лучших ходов.
        return res;
    }

private:

    // Эта функция выполняет ход на доске, обновляя её состояние в соответствии с указанным ходом.
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        // Если ход включает битую фигуру, устанавливает её позицию на доске в 0 (убирает фигуру).
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
        // Если фигура достигает противоположного края доски, превращает её в дамку (увеличивает значение на 2).
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        // Перемещает фигуру на новую позицию.
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        // Устанавливает старую позицию фигуры в 0 (убирает фигуру с начальной позиции).
        mtx[turn.x][turn.y] = 0;
        // Возвращает обновленное состояние доски.
        return mtx;
    }

    // Эта функция вычисляет оценку текущего состояния доски для бота, учитывая количество и потенциал фигур.
    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        // color - who is max player
        // color - определяет, какой игрок максимизирует очки (первый бот или второй)
        // Переменные для подсчета количества белых и черных шашек и дамок
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            // Подсчитывает количество белых и черных шашек и дамок на доске
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);
                // Добавляет вес к оценке в зависимости от потенциала шашек (близость к превращению в дамку)
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color)
        {
            // Если первый бот играет черными, меняет местами количество белых и черных фигур
            swap(b, w);
            swap(bq, wq);
        }

        // Если у белых нет фигур, возвращает бесконечность (что означает проигрыш)
        if (w + wq == 0)
            return INF;

        // Если у черных нет фигур, возвращает 0 (что означает выигрыш)
        if (b + bq == 0)
            return 0;

        // Устанавливает коэффициент для дамок в зависимости от режима оценки
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }

        // Возвращает отношение количества черных фигур к белым, умноженное на коэффициент для дамок
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

    // Эта функция использует рекурсивный подход для поиска лучшего хода, учитывая возможные ходы с битьем и без битья. 
    // Она инициализирует переменные для хранения лучших ходов и состояний, 
    // анализирует каждый возможный ход и обновляет лучший ход и его состояние, если текущий ход имеет лучшую оценку.
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    {   
        // Если нет ходов, вычисления идут дальше
        next_best_state.push_back(-1);
        next_move.emplace_back(-1, -1, -1, -1);
        // хранит лучшую оценку найденного хода
        // начальное значение -1 указывает на отсутствие лучшего хода.
        double best_score = -1;

        // Поиск возможных ходов
        if (state != 0)
            find_turns(x, y, mtx);
        auto turns_now = turns;

        // сохраняет текущее состояние флага have_beats, указывающего, есть ли возможность битья фигур
        bool have_beats_now = have_beats;

        // Проверка наличия возможности битья фигуры
        if (!have_beats_now && state != 0)
        {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        // Инициализация векторов для лучших ходов и состояний
        vector<move_pos> best_moves;
        vector<int> best_states;

        // Анализ каждого возможного хода
        for (auto turn : turns_now)
        {
            // Рекурсивный поиск лучшего хода
            size_t next_state = next_move.size();
            double score; // хранит оценку текущего хода

            // Оценка хода с битьем фигур
            if (have_beats_now)
            {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else
                // Оценка хода без битья фигур
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }
            // Обновление лучшего хода и состояния
            if (score > best_score)
            {
                best_score = score;
                next_best_state[state] = (have_beats_now ? int(next_state) : -1);
                next_move[state] = turn;
            }
        }
        // Возвращает лучшую оценку
        return best_score;
    }

    // использует рекурсивный подход для поиска лучшего хода, анализируя возможные ходы с битьем и без битья 
    // обновляет лучшую оценку, если текущий ход имеет лучшую оценку
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {   
        // проверяет, достигнута ли максимальная глубина поиска
        // глубина бывает четная и нечетная
        // в случае если глубина четная - ходит соперник, еслии нечетная - значит ходим мы
        if (depth == Max_depth)
        {
            // если да, - оценивает текущее состояния доски
            return calc_score(mtx, (depth % 2 == color));
        }
        // если координаты (x, y) не равны -1, 
        // вызывает find_turns для поиска всех возможных ходов для фигуры на позиции x,y на доске mtx
        if (x != -1)
        {
            find_turns(x, y, mtx);
        }
        //в противном случае вызывает find_turns для поиска всех возможных ходов для цвета
        else
            find_turns(color, mtx);

        // Сохранение текущих ходов и состояний
        auto turns_now = turns;
        bool have_beats_now = have_beats;

        // Проверка наличия возможности битья фигур
        if (!have_beats_now && x != -1)
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }
        
        // Проверка наличия возможных ходов
        if (turns.empty())
            // возвращает оценку в зависимости от глубины depth
            // Если глубина четная, возвращает 0 (проигрыш), иначе возвращает INF (выигрыш)
            // для обработки ситуации, когда нет возможных ходов
            return (depth % 2 ? 0 : INF);

        // переменные для оценки ходов
        double min_score = INF + 1; // хранит минимальную оценку найденного хода; указывает на отсутствие минимальной оценки
        double max_score = -1; // хранит максимальную оценку найденного хода; указывает на отсутствие максимальной оценки

        // Оценка хода без битья
        for (auto turn : turns_now)
        {
            double score = 0.0;
            if (!have_beats_now && x == -1)
            {
                // для оценки текущего хода без битья фигуры
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else
            {
                // для оценки текущего хода с битьем фигуры
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            // Обновление минимальной и максимальной оценки
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            // alpha-beta pruning
            //Альфа-бета отсечение

            // если глубина - четная, то обновляет альфа, иначе обновляет бета
            if (depth % 2)
                alpha = max(alpha, max_score);
            else
                beta = min(beta, min_score);

            // если уровень optimization не равен "O0" и альфа больше или равно бета -
            // возвращает оценку в зависимости от глубины depth
            if (optimization != "O0" && alpha >= beta)
                return (depth % 2 ? max_score + 1 : min_score - 1);
        }
        // Возвращение лучшей оценки в завс-ти от глубины depth
        return (depth % 2 ? max_score : min_score);
    }

public:
    // Эта перегруженная функция вызывает другую версию find_turns(), передавая ей цвет фигуры и текущее состояние доски.
    // Она используется для поиска всех возможных ходов для фигур заданного цвета на текущей доске.
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    // Эта перегруженная функция вызывает другую версию find_turns(), передавая ей координаты клетки и текущее состояние доски.
    // Она используется для поиска всех возможных ходов для фигуры, находящейся на указанной клетке.
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    // Вызывает find_turns() для каждой фигуры заданного цвета, чтобы найти все возможные ходы.
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        // Перемешивает найденные ходы для случайного выбора.
        shuffle(turns.begin(), turns.end(), rand_eng);
        // Устанавливает флаг наличия битых фигур.
        have_beats = have_beats_before;
    }

    // Эта функция ищет все возможные ходы для фигур заданного цвета на доске mtx.
    // Она использует другую версию find_turns() для поиска ходов для каждой фигуры.

    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear();
        have_beats = false;

        // Определяет тип фигуры на клетке (x, y).
        POS_T type = mtx[x][y];
        // check beats
        // Проверка возможности битья фигуры
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            // Проверка для обычных шашек
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    // Добавляет ход с битьем в список возможных ходов.
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens
            // Проверка для дамок
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            // Добавляет ход с битьем для дамки в список возможных ходов.
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        // Проверка других возможных ходов
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            // Проверка для обычных шашек
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                // Добавляет обычный ход в список возможных ходов.
                turns.emplace_back(x, y, i, j);
            }
            break;
        }
        default:
            // check queens
            // Проверка для дамок
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        // Добавляет обычный ход для дамки в список возможных ходов
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

public:
    // Вектор, содержащий все возможные ходы для текущего состояния доски
    vector<move_pos> turns;

    // Флаг, указывающий, есть ли в текущем состоянии доски возможность битья фигур
    bool have_beats;

    // Максимальная глубина поиска для алгоритма поиска ходов
    int Max_depth;

private:
    // Генератор случайных чисел, используемый для перемешивания ходов
    default_random_engine rand_eng;

    // Режим оценки, определяющий, как рассчитывается оценка состояния доски (например, с учетом потенциала шашек)
    string scoring_mode;

    // Уровень оптимизации, влияющий на производительность алгоритма поиска ходов (например, использование альфа-бета отсечения)
    string optimization;

    // Вектор, содержащий последовательность лучших ходов, найденных для бота
    vector<move_pos> next_move;

    // Вектор, содержащий индексы следующих состояний в последовательности лучших ходов
    vector<int> next_best_state;

    // Указатель на объект Board, представляющий текущую игровую доску
    Board* board;
    Config* config; // Указатель на объект Config, содержащий настройки игры и бота
};

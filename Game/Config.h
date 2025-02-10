#pragma once // нестандартная, но широко распространённая препроцессорная директива, разработанная для контроля за тем, чтобы конкретный исходный файл при компиляции подключался строго один раз
#include <fstream> // библиотека для работы с файловыми потоками
#include <nlohmann/json.hpp> // библиотека для работы с JSON
// для удобства использования библиотеки
using json = nlohmann::json;

// Подключает заголовочный файл, содержащий путь к проекту (project_path)
#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        // // Конструктор класса Config, который вызывает метод reload() для загрузки настроек
        reload();
    }

    void reload()
    {
        // // Открывает файл settings.json для чтения
        std::ifstream fin(project_path + "settings.json");

        // Считывает содержимое файла в объект config
        fin >> config;

        // Закрывает файловый поток
        fin.close();
    }

    // Метод для перезагрузки конфигурации из файла settings.json
    // Перегрузка оператора () для доступа к настройкам по имени
    // const в конце объявления указывает, что этот метод не изменяет состояние объекта Config, а только получает
    // Перегрузка оператора () позволяет использовать объект Config как функцию для доступа к настройкам
    // например setting_dir - это секция "Bot", а setting_name - это название определенной настройки в этой секции, например "IsWhiteBot"
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        // Этот оператор возвращает значение из JSON config, используя переданные параметры setting_dir и setting_name для доступа к соответствующему элементу
        return config[setting_dir][setting_name];
    }

  private:
    json config;
    // Объект для хранения конфигурации в формате JSON
};

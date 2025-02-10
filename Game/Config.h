#pragma once // �������������, �� ������ ��������������� ��������������� ���������, ������������� ��� �������� �� ���, ����� ���������� �������� ���� ��� ���������� ����������� ������ ���� ���
#include <fstream> // ���������� ��� ������ � ��������� ��������
#include <nlohmann/json.hpp> // ���������� ��� ������ � JSON
// ��� �������� ������������� ����������
using json = nlohmann::json;

// ���������� ������������ ����, ���������� ���� � ������� (project_path)
#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        // // ����������� ������ Config, ������� �������� ����� reload() ��� �������� ��������
        reload();
    }

    void reload()
    {
        // // ��������� ���� settings.json ��� ������
        std::ifstream fin(project_path + "settings.json");

        // ��������� ���������� ����� � ������ config
        fin >> config;

        // ��������� �������� �����
        fin.close();
    }

    // ����� ��� ������������ ������������ �� ����� settings.json
    // ���������� ��������� () ��� ������� � ���������� �� �����
    // const � ����� ���������� ���������, ��� ���� ����� �� �������� ��������� ������� Config, � ������ ��������
    // ���������� ��������� () ��������� ������������ ������ Config ��� ������� ��� ������� � ����������
    // �������� setting_dir - ��� ������ "Bot", � setting_name - ��� �������� ������������ ��������� � ���� ������, �������� "IsWhiteBot"
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        // ���� �������� ���������� �������� �� JSON config, ��������� ���������� ��������� setting_dir � setting_name ��� ������� � ���������������� ��������
        return config[setting_dir][setting_name];
    }

  private:
    json config;
    // ������ ��� �������� ������������ � ������� JSON
};

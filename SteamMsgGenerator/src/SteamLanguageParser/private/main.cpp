
#include "SteamLanguageParser.h"
#include <string_convert.h>
#include <FunctionExitHelper.h>
#include <cxxopts.hpp>
#include <filesystem>
int main(int argc, char** argv) {
    std::filesystem::current_path(std::filesystem::path(R"(C:\Project\sonkwo_client_runtime\build\_deps\steamkit-src\Resources\SteamLanguage)"));
    bool bShowHelp{ false };
    std::string cwd = ConvertU8ViewToString(std::filesystem::current_path().u8string());
    cxxopts::Options options("SteamLanguageParser", "generator c++ files");
    options.positional_help("[steam_language_files] ").show_positional_help();
    options.add_options()
        ("o,outdir", "out dir", cxxopts::value<std::string>())
        ("p,projectdir", "project dir", cxxopts::value<std::string>()->default_value(cwd))
        ("e,enumfile", "one enum file", cxxopts::value<std::string>())
        ("steam_language_files", "steam_language_files", cxxopts::value<std::vector<std::string>>())
        ("h,help", "print usage")
        ;

    options.parse_positional({ "steam_language_files"});
    FunctionExitHelper_t exiter([&]() {
        if (bShowHelp) {
            std::cout << options.help() << std::endl;
        }
        });

    auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
        bShowHelp = true;
        return 0;
    }
    if (result.count("outdir") == 0) {
        bShowHelp = true;
        return -1;
    }
    if (result.count("steam_language_files") == 0) {
        bShowHelp = true;
        return 0;
    }
    auto steamFiles=result["steam_language_files"].as<std::vector<std::string>>();
    FSteamLanguageParser parser;
    auto bres=parser.Init(ConvertStringToU8View(result["projectdir"].as<std::string>()),ConvertStringToU8View(result["outdir"].as<std::string>()));
    if (!bres) {
        return -1;
    }
    if (result.count("enumfile")) {
        bres = parser.EmitEnumToOneFile(ConvertStringToU8View(result["enumfile"].as<std::string>()));
        if (!bres) {
            return -1;
        }
    }
    for (const auto& filePath : steamFiles) {
        auto [ptr, binsert] = parser.AddFileToParse(ConvertStringToU8View(filePath));
        if (!ptr ) {
            std::cerr << "Failed to add file: " << filePath << std::endl;
            return -1;
        }
    }
    bres=parser.Parse();
    if (!bres) {
        return -1;
    }
    bres=parser.EmitCode();
    if (!bres) {
        return -1;
    }
    return 0;

}

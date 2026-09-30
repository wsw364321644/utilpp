
#include "dir_util.h"
#include "dir_util_internal.h"
#include <filesystem>
#include <string>
bool DirUtil::CreateShortcut(ShortcutOptions_t ShortcutOptions)
{
    std::string cmd = std::format("osascript -e 'tell application \"Finder\" to make new alias at (POSIX file \"{}\") to (POSIX file \"{}\")'"
        , ConvertU8ViewToView(ShortcutOptions.TargetPath)
        , ConvertU8ViewToView(ShortcutOptions.ShortcutPath)
    );
    return system(cmd.c_str()) == 0;
}
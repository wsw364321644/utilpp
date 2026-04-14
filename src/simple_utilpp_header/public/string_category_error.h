#pragma once
#include <string>
#include <string_view>
#include <system_error>
class string_category_error_category : public std::error_category {
    public:
        [[nodiscard]] virtual const char* name() const noexcept override {
            return CategoryName.c_str();
        }

        [[nodiscard]] virtual std::string message(int _Errval) const override {
            return ErrorMessage;
        }

    explicit string_category_error_category(std::string_view name,std::string_view msg) : CategoryName(name), ErrorMessage(msg){}
private:
    std::string CategoryName;
    std::string ErrorMessage;

};

[[nodiscard]] inline const std::error_code make_string_category_error(int val, std::string_view cat_name, std::string_view msg="") {
    return std::error_code(val, string_category_error_category(cat_name, msg));
}

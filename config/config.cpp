#include "config.h"
#include <toml.hpp>

using namespace std;

namespace HuiFu
{
    Config &Config::get_instance()
    {
        static Config c;
        return c;
    }

    bool Config::init(const string &filename)
    {
        using namespace toml;
        try
        {
            const auto data = parse(filename);
            const auto &xtp = find(data, "xtp");
            const auto &system = find(xtp, "system");
            const auto &trader = find(xtp, "trader");
            const auto &quote = find(xtp, "quote");
            const auto &accounts = find(data, "accounts");

            mSystem = {
                find<uint8_t>(system, "client_id"),
                find<std::string>(system, "log_path"),
                find<int>(system, "log_level"),
                find<int>(system, "resume_type"),
                find<int32_t>(system, "hb_interval")};

            mTrader = {
                find<std::string>(trader, "version"),
                find<std::string>(trader, "software_key"),
                find<std::string>(trader, "ip"),
                find<int>(trader, "port")};

            mQuote = {
                find<std::string>(quote, "ip"),
                find<int>(quote, "port")};

            const auto accounts_table = find<std::vector<table>>(accounts, "table");
            for (auto &account : accounts_table)
            {
                mAccounts.emplace_back(
                    toml::get<std::string>(account.at("user")),
                    toml::get<std::string>(account.at("password")));
            }
        }
        catch (const syntax_error &e)
        {
            return false;
        }

        return true;
    }

    const SystemConfig &Config::get_system_config() const
    {
        return mSystem;
    }
    const TraderConfig &Config::get_trader_config() const
    {
        return mTrader;
    }
    const QuoteConfig &Config::get_quote_config() const
    {
        return mQuote;
    }
    const vector<AccountConfig> &Config::get_accounts_config() const
    {
        return mAccounts;
    }
} // namespace HuiFu
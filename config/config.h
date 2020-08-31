#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

namespace HuiFu
{
    struct SystemConfig
    {
        uint8_t client_id;
        std::string log_path;
        int log_level;
        int resume_type;
        int32_t heart_beat_interval;
    };

    struct TraderConfig
    {
        std::string version;
        std::string software_key;
        std::string ip;
        int port;
    };

    struct QuoteConfig
    {
        std::string ip;
        int port;
    };

    struct AccountConfig
    {
        std::string user;
        std::string password;

        AccountConfig(std::string user, std::string password) : user(std::move(user)), password(std::move(password)) {}
    };

    class Config
    {
    private:
        Config() = default;
        ~Config() = default;

        SystemConfig mSystem;
        TraderConfig mTrader;
        QuoteConfig mQuote;
        // 账户的初始化位置决定了账户信息的获取位置
        std::vector<AccountConfig> mAccounts;

    public:
        static Config &get_instance();
        bool init(const std::string &);
        bool reload(const std::string &);

        const SystemConfig &get_system_config() const;
        const TraderConfig &get_trader_config() const;
        const QuoteConfig &get_quote_config() const;
        const std::vector<AccountConfig> &get_accounts_config() const;
        size_t get_account_num() const { return mAccounts.size(); }
    };
} // namespace HuiFu

#endif
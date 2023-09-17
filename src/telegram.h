#include <FastBot.h>

#include <credentials.h>
#include <fan_control.h>
#include <light.h>
#include <sensors.h>

int menu_id;
int info_id;
int value_id;
int welcome_id;
int status_id;
int depth = 0;

int test_c = 0;

int location = 0;

float val;
int val_success;
bool restart = false;

unsigned int telegram_status_cycle = 30 * 60 * 1000;
unsigned int telegram_tick_cycle = 2000;
unsigned int telegram_restart_cycle = 46 * 60 * 60 * 1000;
unsigned long telegram_restart_timestamp;
unsigned long telegram_timestamp;

String main_menu;
String fan_menu;
String light_menu;
String welcome_text;
String new_value_text;
String status;
String light_presets;

void setupTelegram();
void updateStatus();
void restartBot();

FastBot bot(TELGRAM_BOT_TOKEN);

String menuText(String menu)
{
    if (depth > 0)
    {
        return menu + "\nHome";
    }

    else
    {
        return menu;
    }
}

void delUserMsg(int id)
{
    if (id == menu_id || id == info_id)
    {
        return;
    }

    else
    {
        bot.deleteMessage(id);
    }
}

void wrongValue()
{
    bot.editMessage(info_id, "Value not in appropriate range! Send new value.");
    bot.editMenu(menu_id, "Home");
    depth = 99;
}

void goMainMenu()
{
    bot.editMessage(info_id, welcome_text);
    bot.editMenu(menu_id, main_menu);
    depth = 0;
    location = 0;
}

void delAllBotMsg()
{
    bot.deleteMessage(welcome_id);
    bot.deleteMessage(menu_id);
    bot.deleteMessage(info_id);
    bot.deleteMessage(status_id);
}

void changeValue(String msg)
{
    float buffer_val;
    bool val_success = false;
    if (depth == 99)
    {
        buffer_val = msg.toFloat();
        if (buffer_val != 0)
        {
            val = buffer_val;
            bot.editMessage(info_id, "New value: " + String(val));
            bot.editMenu(menu_id, menuText("Accept"));
            depth = 101;
        }

        else
        {
            delUserMsg(bot.lastUsrMsg());
        }
    }

    if (depth == 101)
    {
        delUserMsg(value_id);

        if (msg == "Accept")
        {
            switch (location)
            {
            case 1:
                if (0 <= val && val <= 100)
                {
                    if (val <= 5)
                    {
                        val = 0;
                    }
                    target_pwm_exhaust = val * 0.01;
                    val_success = true;
                }
                break;

            case 2:
                if (0 <= val && val <= 100)
                {
                    if (val <= 5)
                    {
                        val = 0;
                    }
                    target_pwm_circ = val * 0.01;
                    val_success = true;
                }
                break;

            case 3:
                if (0 < val && val < 48)
                {
                    updateCycle(val, cycle_off);
                    val_success = true;
                }
                break;

            case 4:
                if (0 < val && val < 48)
                {
                    updateCycle(val, cycle_on);
                    val_success = true;
                }
                break;

            default:
                break;
            }

            Serial.println("Success: " + String(val_success));

            if (val_success == 1)
            {
                updateStatus();
                goMainMenu();
            }
            else
            {
                wrongValue();
            }
        }
    }
}

void updateStatusMsg()
{
    status = "__Current Status__\nTemperature: *" + String(bme280_meas[0]) + " C*\nHumidity: *" + String(bme280_meas[1]) + " %*\nSoil moisture: *" +
             String(soil_meas) + " units*\nWater Detection: *" + String(water_meas) + " units*\nExhaust Fan: *" + String(target_pwm_exhaust * 100) +
             " %*\nCirculation Fan: *" + String(target_pwm_circ * 100) + " %*\nCycle Duration \\(Day \\| Night \\| Total\\): *" + String(cycle_on) + " \\| " + String(cycle_off) + " \\| " + String(cycle_total) + " hours*";
}

void updateStatus()
{
    updateStatusMsg();
    bot.editMessage(status_id, status);
    telegram_timestamp = millis();
}

void changeValuePrompt(int loc)
{
    depth = 99;
    bot.editMessage(info_id, new_value_text);
    bot.editMenu(menu_id, "Home");
    location = loc;
}

void newMsg(FB_msg &msg)
{
    if (msg.userID == TELEGRAM_USER_ID)
    {
        switch (depth)
        {

        case 0:
            if (msg.data == "Fan Settings")
            {
                depth = 1;
                bot.editMessage(info_id, "__Current settings__\nExhaust Fan:\t*" + String(current_pwm_exhaust * 100) + " %*\nCirculation Fan:\t*" + String(current_pwm_circ * 100) + "%*");
                bot.editMenu(menu_id, menuText(fan_menu));
            }

            else if (msg.data == "Light Settings")
            {
                depth = 1;
                bot.editMessage(info_id, "__Current settings__\nDay:\t" + String(cycle_on) + " hours\nNight:\t" + String(cycle_off) +
                                             " hours\nTotal Day/Night Cycle: " + String(cycle_off + cycle_on) + " hours");
                bot.editMenu(menu_id, menuText(light_menu));
            }
            break;

        case 1:
            if (msg.data == "Exhaust")
            {
                changeValuePrompt(1);
            }

            else if (msg.data == "Circulation")
            {
                changeValuePrompt(2);
            }

            else if (msg.data == "Day")
            {
                changeValuePrompt(3);
            }

            else if (msg.data == "Night")
            {
                changeValuePrompt(4);
            }

            else if (msg.data == "Presets")
            {
                depth = 2;
                bot.editMenu(menu_id, menuText(light_presets));
            }
            break;

        case 2:
            if (msg.data == "12/12")
            {
                updateCycle(12, 12);
                updateStatus();
                goMainMenu();
            }

            else if (msg.data == "16/8")
            {
                updateCycle(16, 8);
                updateStatus();
                goMainMenu();
            }
            else if (msg.data == "18/6")
            {
                updateCycle(18, 6);
                updateStatus();
                goMainMenu();
            }
            break;

        case 99:
            value_id = bot.lastUsrMsg();
            changeValue(msg.text);
            break;

        case 101:
            changeValue(msg.data);
            break;
        }

        if (msg.data == "Home" && depth > 0)
        {
            goMainMenu();
        }

        else if (msg.text == REMOTE_RESTART_KEY)
        {
            delAllBotMsg();
            bot.deleteMessage(bot.lastUsrMsg());
            restart = true;
        }

        else if (msg.text == "/status")
        {
            bot.deleteMessage(msg.ID);
            updateStatus();
        }

        else if (msg.text == BOT_RESTART_KEY)
        {
            bot.deleteMessage(msg.ID);
            restartBot();
        }

        else
        {
            delUserMsg(msg.ID);
        }
    }
}

void startMenu()
{
    depth = 0;
    bot.inlineMenu("Which parameter should be changed?", menuText(main_menu));
    menu_id = bot.lastBotMsg();
    bot.sendMessage(welcome_text);
    info_id = bot.lastBotMsg();
}

void restartBot()
{
    delAllBotMsg();
    setupTelegram();
    updateStatus();
}

void setupTelegram()
{
    bot.setChatID(TELEGRAM_USER_ID);
    bot.attach(newMsg);
    bot.setTextMode(FB_MARKDOWN);
    bot.setPeriod(telegram_tick_cycle);

    updateStatusMsg();

    main_menu = "Fan Settings\n Light Settings";
    fan_menu = "Exhaust\tCirculation";
    light_menu = "Day\tNight\tPresets";
    light_presets = "12/12\t16/8\t18/6";
    welcome_text = "Choose an option above";
    new_value_text = "Send the new value now \\(cannot be 0\\)";

    telegram_restart_timestamp = millis();

    bot.sendMessage("Hi\\! I am now online.");
    welcome_id = bot.lastBotMsg();
    bot.sendMessage("Status coming soon...");
    status_id = bot.lastBotMsg();
    startMenu();
}
#include "hold_games.h"

#include "assets/assets.h"
#include "building/count.h"
#include "city/constants.h"
#include "city/data_private.h"
#include "city/festival.h"
#include "city/finance.h"
#include "city/games.h"
#include "city/gods.h"
#include "core/image_group.h"
#include "game/resource.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/text.h"
#include "graphics/panel.h"
#include "graphics/window.h"
#include "input/input.h"
#include "translation/translation.h"
#include "window/advisors.h"
#include "window/city.h"
#include "window/message_dialog.h"

static void button_game(int god, int param2);
static void button_help(int param1, int param2);
static void button_close(int param1, int param2);
static void button_hold_games(int param1, int param2);

static image_button image_buttons_bottom[] = {
    {59, 353, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help, button_none, 0, 0, 1},
    {558, 352, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close, button_none, 0, 0, 1},
    {494, 350, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 4, button_close, button_none, 0, 0, 1}
};

static image_button action_button[] = {
    {452, 350, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 0, button_hold_games, button_none, 1, 0, 1}
};

static generic_button buttons_games_size[] = {
    {170, 96, 80, 90, button_game, button_none, 1, 0},
    {270, 96, 80, 90, button_game, button_none, 2, 0},
    {370, 96, 80, 90, button_game, button_none, 3, 0},
    //{370, 96, 80, 90, button_game, button_none, 4, 0},
};

static struct {
    int focus_button_id;
    int focus_image_button_id;
    int focus_game_button_id;
    int return_to_city;
    int game_possible;
} data;

static void draw_background(void)
{
    int selected_game_id = city_data.games.selected_games_id;
    if (!selected_game_id) {
        city_data.games.selected_games_id = 1;
        selected_game_id = 1;
    }
    games_type *game = city_games_get_game_type(selected_game_id);

    window_advisors_draw_dialog_background();
    graphics_in_dialog();

    outer_panel_draw(48, 48, 34, 22);
    text_draw_centered(translation_for(game->header_key), 48, 60, 544, FONT_LARGE_BLACK, 0);
    for (int i = 0; i < MAX_GAMES; i++) {
        if (i == game->id - 1) {
            button_border_draw(100 * i + 165, 92, 90, 100, 1);
            image_draw(assets_get_image_id(assets_get_group_id("Areldir", "UI_Elements"),
                "Naum Ico S") + (2 * i), 100 * i + 170, 96);
        } else {
            image_draw(assets_get_image_id(assets_get_group_id("Areldir", "UI_Elements"),
                "Naum Ico DS") + (2 * i), 100 * i + 170, 96);
        }
    }
    text_draw_multiline(translation_for(game->description_key), 70, 222, 500, FONT_NORMAL_BLACK, 0);

    int width = text_draw(translation_for(TR_WINDOW_GAMES_COST), 120, 300, FONT_NORMAL_BLACK, 0);
    width += text_draw_money(city_games_money_cost(selected_game_id), 120 + width, 300, FONT_NORMAL_BLACK);
    text_draw(translation_for(TR_WINDOW_GAMES_PERSONAL_FUNDS), 120 + width, 300, FONT_NORMAL_BLACK, 0);

    width = 0;
    int has_resources = 1;
    int resource_cost = 0;
    for (int i = 0; i < RESOURCE_MAX; ++i) {
        resource_cost = city_games_resource_cost(selected_game_id,i);
        if (resource_cost) {
            width += text_draw_number(resource_cost, '@', "", 120 + width, 320, FONT_NORMAL_BLACK);
            if (city_resource_count(i) < resource_cost) {
                has_resources = 0;
            }
            image_draw(image_group(GROUP_RESOURCE_ICONS) + i, 120 + width, 316);
            width += 32;
        }
    }
    data.game_possible = 0;

    if (!building_count_active(game->building_id_required)) {
        text_draw(translation_for(TR_WINDOW_GAMES_NO_VENUE), 130, 352, FONT_NORMAL_BLACK, 0);
    } else if (city_emperor_personal_savings() < city_games_money_cost(selected_game_id)) {
        text_draw(translation_for(TR_WINDOW_GAMES_NOT_ENOUGH_FUNDS), 130, 352, FONT_NORMAL_BLACK, 0);
    } else if (!has_resources) {
        text_draw(translation_for(TR_WINDOW_GAMES_NOT_ENOUGH_RESOURCES), 130, 352, FONT_NORMAL_BLACK, 0);
    } else {
        data.game_possible = 1;
        image_buttons_draw(0, 0, action_button, 1);
    }

    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();
    image_buttons_draw(0, 0, image_buttons_bottom, 3);
    if (data.game_possible) {
        image_buttons_draw(0, 0, action_button, 1);
    }
    graphics_reset_dialog();
}

static void close_window()
{
    if (data.return_to_city) {
        window_city_show();
    } else {
        window_advisors_show();
    }
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog(m);
    int handled = 0;
    handled |= image_buttons_handle_mouse(m_dialog, 0, 0, image_buttons_bottom, 3, &data.focus_image_button_id);
    handled |= image_buttons_handle_mouse(m_dialog, 0, 0, action_button, 1, &data.focus_game_button_id);
    handled |= generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_games_size, 3, &data.focus_button_id);
    if (data.focus_image_button_id || data.focus_game_button_id) {
        data.focus_button_id = 0;
    }
    if (!handled && input_go_back_requested(m, h)) {
        close_window();
    }
}

static void button_game(int game, int param2)
{
    city_data.games.selected_games_id = game;
    window_invalidate();
}

static void button_help(int param1, int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_ADVISOR_ENTERTAINMENT, 0);
}

static void button_close(int param1, int param2)
{
    close_window();
}

static void button_hold_games(int param1, int param2)
{
    if (data.game_possible) {
        city_games_schedule(city_data.games.selected_games_id);
        close_window();
    }
}

static void get_tooltip(tooltip_context *c)
{
    if (!data.focus_image_button_id && !data.focus_game_button_id && (!data.focus_button_id || data.focus_button_id > 4)) {
        return;
    }

    c->type = TOOLTIP_BUTTON;

    if (data.focus_image_button_id) {
        switch (data.focus_image_button_id) {
            case 1: c->text_id = 1; break;
            case 2: c->text_id = 2; break;
            case 3: c->translation_key = TR_TOOLTIP_NO_GAME; break;
        }
    } else if (data.focus_game_button_id && data.game_possible) {
        c->translation_key = TR_TOOLTIP_HOLD_GAME;
    } else if (data.focus_button_id) {
        games_type *game = city_games_get_game_type(data.focus_button_id);
        if (game) {
            c->translation_key = game->header_key;
        }
    } else {
        c->type = TOOLTIP_NONE;
    }
}

void window_hold_games_show(int return_to_city)
{
    window_type window = {
        WINDOW_HOLD_GAMES,
        draw_background,
        draw_foreground,
        handle_input,
        get_tooltip
    };
    data.return_to_city = return_to_city;
    window_show(&window);
}

#include "../flipbip.h"
#include <lib/toolbox/value_index.h>

// enum SettingsIndex {
//     SettingsIndexBip39Strength = 10,
//     SettingsIndexBip44Coin,
//     SettingsIndexHaptic,
//     SettingsIndexValue1,
// };

const char* const haptic_text[2] = {
    "OFF",
    "ON",
};
const uint32_t haptic_value[2] = {
    FlipBipHapticOff,
    FlipBipHapticOn,
};

const char* const led_text[2] = {
    "OFF",
    "ON",
};
const uint32_t led_value[2] = {
    FlipBipLedOff,
    FlipBipLedOn,
};

const char* const bip39_strength_text[3] = {
    "12",
    "18",
    "24",
};
const uint32_t bip39_strength_value[3] = {
    FlipBipStrength128,
    FlipBipStrength192,
    FlipBipStrength256,
};

const char* const bip44_coin_text[2] = {
    "BTC",
    "ETH",
};
const uint32_t bip44_coin_value[2] = {
    FlipBipCoinBTC0,
    FlipBipCoinETH60,
};

static void flipbip_scene_settings_set_haptic(VariableItem* item) {
    FlipBip* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, haptic_text[index]);
    app->haptic = haptic_value[index];
}

static void flipbip_scene_settings_set_led(VariableItem* item) {
    FlipBip* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, led_text[index]);
    app->led = led_value[index];
}

static void flipbip_scene_settings_set_bip39_strength(VariableItem* item) {
    FlipBip* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, bip39_strength_text[index]);
    app->bip39_strength = bip39_strength_value[index];
}

static void flipbip_scene_settings_set_bip44_coin(VariableItem* item) {
    FlipBip* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, bip44_coin_text[index]);
    app->bip44_coin = bip44_coin_value[index];
}

void flipbip_scene_settings_submenu_callback(void* context, uint32_t index) {
    FlipBip* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void flipbip_scene_settings_on_enter(void* context) {
    FlipBip* app = context;
    VariableItem* item;
    uint8_t value_index;

    // BIP39 strength
    item = variable_item_list_add(
        app->variable_item_list, "BIP39 Words:", 3, flipbip_scene_settings_set_bip39_strength, app);
    value_index = value_index_uint32(app->bip39_strength, bip39_strength_value, 3);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, bip39_strength_text[value_index]);

    // BIP44 Coin
    item = variable_item_list_add(
        app->variable_item_list, "BIP44 Coin:", 2, flipbip_scene_settings_set_bip44_coin, app);
    value_index = value_index_uint32(app->bip44_coin, bip44_coin_value, 2);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, bip44_coin_text[value_index]);

    // Vibro on/off
    item = variable_item_list_add(
        app->variable_item_list, "Vibro/Haptic:", 2, flipbip_scene_settings_set_haptic, app);
    value_index = value_index_uint32(app->haptic, haptic_value, 2);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, haptic_text[value_index]);

    // LED Effects on/off
    item = variable_item_list_add(
        app->variable_item_list, "LED FX:", 2, flipbip_scene_settings_set_led, app);
    value_index = value_index_uint32(app->led, led_value, 2);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, led_text[value_index]);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipBipViewIdSettings);
}

bool flipbip_scene_settings_on_event(void* context, SceneManagerEvent event) {
    FlipBip* app = context;
    UNUSED(app);
    bool consumed = false;
    if(event.type == SceneManagerEventTypeCustom) {
    }
    return consumed;
}

void flipbip_scene_settings_on_exit(void* context) {
    FlipBip* app = context;
    variable_item_list_set_selected_item(app->variable_item_list, 0);
    variable_item_list_reset(app->variable_item_list);
}
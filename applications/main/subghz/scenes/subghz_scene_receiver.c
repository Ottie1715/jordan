#include "../subghz_i.h"
#include "../views/receiver.h"
#include <dolphin/dolphin.h>
#include <lib/subghz/protocols/bin_raw.h>

#define TAG "SubGhzSceneReceiver"

const NotificationSequence subghz_sequence_rx = {
    &message_green_255,

    &message_display_backlight_on,

    &message_vibro_on,
    &message_note_c6,
    &message_delay_50,
    &message_sound_off,
    &message_vibro_off,

    &message_delay_50,
    NULL,
};

const NotificationSequence subghz_sequence_rx_locked = {
    &message_green_255,

    &message_display_backlight_on,

    &message_vibro_on,
    &message_note_c6,
    &message_delay_50,
    &message_sound_off,
    &message_vibro_off,

    &message_delay_500,

    &message_display_backlight_off,
    NULL,
};

static void subghz_scene_receiver_update_statusbar(void* context) {
    SubGhz* subghz = context;
    FuriString* history_stat_str = furi_string_alloc();
    if(!subghz_history_get_text_space_left(subghz->history, history_stat_str)) {
        FuriString* frequency_str = furi_string_alloc();
        FuriString* modulation_str = furi_string_alloc();

#ifdef SUBGHZ_EXT_PRESET_NAME
        if(subghz_history_get_last_index(subghz->history) > 0) {
            subghz_get_frequency_modulation(subghz->txrx, frequency_str, modulation_str, false);
        } else {
            FuriString* temp_str = furi_string_alloc();

            subghz_get_frequency_modulation(subghz->txrx, frequency_str, temp_str, true);
            furi_string_printf(
                modulation_str,
                "%s        Mod: %s",
                furi_hal_subghz_get_radio_type() ? "Ext" : "Int",
                furi_string_get_cstr(temp_str));
            furi_string_free(temp_str);
        }
#else
        subghz_get_frequency_modulation(subghz->txrx, frequency_str, modulation_str, false);
#endif

        subghz_view_receiver_add_data_statusbar(
            subghz->subghz_receiver,
            furi_string_get_cstr(frequency_str),
            furi_string_get_cstr(modulation_str),
            furi_string_get_cstr(history_stat_str));

        furi_string_free(frequency_str);
        furi_string_free(modulation_str);
    } else {
        subghz_view_receiver_add_data_statusbar(
            subghz->subghz_receiver, furi_string_get_cstr(history_stat_str), "", "");
        subghz->state_notifications = SubGhzNotificationStateIDLE;
    }
    furi_string_free(history_stat_str);
}

void subghz_scene_receiver_callback(SubGhzCustomEvent event, void* context) {
    furi_assert(context);
    SubGhz* subghz = context;
    view_dispatcher_send_custom_event(subghz->view_dispatcher, event);
}

static void subghz_scene_add_to_history_callback(
    SubGhzReceiver* receiver,
    SubGhzProtocolDecoderBase* decoder_base,
    void* context) {
    furi_assert(context);

    SubGhz* subghz = context;

    FuriString* item_name = furi_string_alloc();
    FuriString* item_time = furi_string_alloc();
    uint16_t idx = subghz_history_get_item(subghz->history);

    SubGhzRadioPreset preset = subghz_get_preset(subghz->txrx);
    if(subghz_history_add_to_history(subghz->history, decoder_base, &preset)) {
        furi_string_reset(item_name);
        furi_string_reset(item_time);

        subghz->state_notifications = SubGhzNotificationStateRxDone;

        subghz_history_get_text_item_menu(subghz->history, item_name, idx);
        subghz_history_get_time_item_menu(subghz->history, item_time, idx);
        subghz_view_receiver_add_item_to_menu(
            subghz->subghz_receiver,
            furi_string_get_cstr(item_name),
            furi_string_get_cstr(item_time),
            subghz_history_get_type_protocol(subghz->history, idx));

        subghz_scene_receiver_update_statusbar(subghz);
    }
    subghz_receiver_reset(receiver);
    furi_string_free(item_name);
    furi_string_free(item_time);
    subghz_rx_key_state_set(subghz, SubGhzRxKeyStateAddKey);
}

void subghz_scene_receiver_on_enter(void* context) {
    SubGhz* subghz = context;

    FuriString* item_name = furi_string_alloc();
    FuriString* item_time = furi_string_alloc();

    if(subghz_rx_key_state_get(subghz) == SubGhzRxKeyStateIDLE) {
        subghz_set_preset(subghz->txrx, "AM650", subghz->last_settings->frequency, NULL, 0);
        subghz_history_reset(subghz->history);
        subghz_rx_key_state_set(subghz, SubGhzRxKeyStateStart);
    }

    subghz_view_receiver_set_lock(subghz->subghz_receiver, subghz_is_locked(subghz));
    subghz_view_receiver_set_mode(subghz->subghz_receiver, SubGhzViewReceiverModeLive);

    //Load history to receiver
    subghz_view_receiver_exit(subghz->subghz_receiver);
    for(uint8_t i = 0; i < subghz_history_get_item(subghz->history); i++) {
        furi_string_reset(item_name);
        furi_string_reset(item_time);
        subghz_history_get_text_item_menu(subghz->history, item_name, i);
        subghz_history_get_time_item_menu(subghz->history, item_time, i);
        subghz_view_receiver_add_item_to_menu(
            subghz->subghz_receiver,
            furi_string_get_cstr(item_name),
            furi_string_get_cstr(item_time),
            subghz_history_get_type_protocol(subghz->history, i));
        subghz_rx_key_state_set(subghz, SubGhzRxKeyStateAddKey);
    }
    furi_string_free(item_name);
    furi_string_free(item_time);
    subghz_scene_receiver_update_statusbar(subghz);
    subghz_view_receiver_set_callback(
        subghz->subghz_receiver, subghz_scene_receiver_callback, subghz);
    subghz_receiver_set_rx_callback(
        subghz->txrx->receiver, subghz_scene_add_to_history_callback, subghz);

    // TODO: Replace with proper solution based on protocol flags, remove kostily and velosipedy from here
    // Needs to be done after subghz refactoring merge!!!
    if(subghz->ignore_starline == true) {
        if(subghz_txrx_load_decoder_by_name_protocol(subghz->txrx, "Star Line")) {
            subghz_protocol_decoder_base_set_decoder_callback(
                subghz_txrx_get_decoder(subghz->txrx), NULL, subghz->txrx->receiver);
        }
    }
    if(subghz->ignore_auto_alarms == true) {
        if(subghz_txrx_load_decoder_by_name_protocol(subghz->txrx, "KIA Seed")) {
            subghz_protocol_decoder_base_set_decoder_callback(
                subghz_txrx_get_decoder(subghz->txrx), NULL, subghz->txrx->receiver);
        }

        if(subghz_txrx_load_decoder_by_name_protocol(subghz->txrx, "Scher-Khan")) {
            subghz_protocol_decoder_base_set_decoder_callback(
                subghz_txrx_get_decoder(subghz->txrx), NULL, subghz->txrx->receiver);
        }
    }
    if(subghz->ignore_magellan == true) {
        if(subghz_txrx_load_decoder_by_name_protocol(subghz->txrx, "Magellan")) {
            subghz_protocol_decoder_base_set_decoder_callback(
                subghz_txrx_get_decoder(subghz->txrx), NULL, subghz->txrx->receiver);
        }
    }

    subghz->state_notifications = SubGhzNotificationStateRx;
    subghz_txrx_stop(subghz->txrx);
    subghz_rx_start(subghz->txrx);
    subghz_view_receiver_set_idx_menu(subghz->subghz_receiver, subghz->idx_menu_chosen);

    //to use a universal decoder, we are looking for a link to it
    furi_check(
        subghz_txrx_load_decoder_by_name_protocol(subghz->txrx, SUBGHZ_PROTOCOL_BIN_RAW_NAME));

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdReceiver);
}

bool subghz_scene_receiver_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;
    bool consumed = false;
    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case SubGhzCustomEventViewReceiverBack:
            // Stop CC1101 Rx
            subghz->state_notifications = SubGhzNotificationStateIDLE;
            subghz_txrx_stop(subghz->txrx);
            subghz_hopper_set_state(subghz->txrx, SubGhzHopperStateOFF);
            subghz->idx_menu_chosen = 0;
            subghz_receiver_set_rx_callback(subghz->txrx->receiver, NULL, subghz);

            if(subghz_rx_key_state_get(subghz) == SubGhzRxKeyStateAddKey) {
                subghz_rx_key_state_set(subghz, SubGhzRxKeyStateExit);
                scene_manager_next_scene(subghz->scene_manager, SubGhzSceneNeedSaving);
            } else {
                subghz_rx_key_state_set(subghz, SubGhzRxKeyStateIDLE);
                subghz_set_preset(
                    subghz->txrx, "AM650", subghz->last_settings->frequency, NULL, 0);
                scene_manager_search_and_switch_to_previous_scene(
                    subghz->scene_manager, SubGhzSceneStart);
            }
            consumed = true;
            break;
        case SubGhzCustomEventViewReceiverOK:
            // Show file info, scene: receiver_info
            subghz->idx_menu_chosen = subghz_view_receiver_get_idx_menu(subghz->subghz_receiver);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReceiverInfo);
            DOLPHIN_DEED(DolphinDeedSubGhzReceiverInfo);
            consumed = true;
            break;
        case SubGhzCustomEventViewReceiverDeleteItem:
            subghz->idx_menu_chosen = subghz_view_receiver_get_idx_menu(subghz->subghz_receiver);

            subghz_history_delete_item(subghz->history, subghz->idx_menu_chosen);
            subghz_view_receiver_delete_element_callback(subghz->subghz_receiver);

            subghz_scene_receiver_update_statusbar(subghz);
            consumed = true;
            break;
        case SubGhzCustomEventViewReceiverConfig:
            subghz->state_notifications = SubGhzNotificationStateIDLE;
            subghz->idx_menu_chosen = subghz_view_receiver_get_idx_menu(subghz->subghz_receiver);
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzViewIdReceiver, SubGhzCustomEventManagerSet);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReceiverConfig);
            consumed = true;
            break;
        case SubGhzCustomEventViewReceiverOffDisplay:
            notification_message(subghz->notifications, &sequence_display_backlight_off);
            consumed = true;
            break;
        case SubGhzCustomEventViewReceiverUnlock:
            subghz_unlock(subghz);
            consumed = true;
            break;
        default:
            break;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        if(subghz_hopper_get_state(subghz->txrx) != SubGhzHopperStateOFF) {
            subghz_hopper_update(subghz->txrx);
            subghz_scene_receiver_update_statusbar(subghz);
        }

        //get RSSI
        SubGhzThresholdRssiData ret_rssi = subghz_threshold_get_rssi_data(subghz->threshold_rssi);

        subghz_receiver_rssi(subghz->subghz_receiver, ret_rssi.rssi);
        subghz_protocol_decoder_bin_raw_data_input_rssi(
            (SubGhzProtocolDecoderBinRAW*)subghz_txrx_get_decoder(subghz->txrx), ret_rssi.rssi);

        switch(subghz->state_notifications) {
        case SubGhzNotificationStateRx:
            notification_message(subghz->notifications, &sequence_blink_cyan_10);
            break;
        case SubGhzNotificationStateRxDone:
            if(!subghz_is_locked(subghz)) {
                notification_message(subghz->notifications, &subghz_sequence_rx);
            } else {
                notification_message(subghz->notifications, &subghz_sequence_rx_locked);
            }
            subghz->state_notifications = SubGhzNotificationStateRx;
            break;
        default:
            break;
        }
    }
    return consumed;
}

void subghz_scene_receiver_on_exit(void* context) {
    UNUSED(context);
}

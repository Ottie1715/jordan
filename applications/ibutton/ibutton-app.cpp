#include "ibutton-app.h"
#include <stdarg.h>
#include <callback-connector.h>
#include <m-string.h>

const char* iButtonApp::app_folder = "ibutton";
const char* iButtonApp::app_extension = ".ibtn";

void iButtonApp::run(void) {
    iButtonEvent event;
    bool consumed;
    bool exit = false;

    scenes[current_scene]->on_enter(this);

    while(!exit) {
        view.receive_event(&event);

        consumed = scenes[current_scene]->on_event(this, &event);

        if(!consumed) {
            if(event.type == iButtonEvent::Type::EventTypeBack) {
                exit = switch_to_previous_scene();
            }
        }
    };

    scenes[current_scene]->on_exit(this);
}

iButtonApp::iButtonApp()
    : fs_api{"sdcard"}
    , sd_ex_api{"sdcard-ex"}
    , notification{"notification"} {
    api_hal_power_insomnia_enter();
    key_worker = new KeyWorker(&ibutton_gpio);

    // we need random
    srand(DWT->CYCCNT);
}

iButtonApp::~iButtonApp() {
    for(std::map<Scene, iButtonScene*>::iterator it = scenes.begin(); it != scenes.end(); ++it) {
        delete it->second;
        scenes.erase(it);
    }

    api_hal_power_insomnia_exit();
}

iButtonAppViewManager* iButtonApp::get_view_manager() {
    return &view;
}

void iButtonApp::switch_to_next_scene(Scene next_scene) {
    previous_scenes_list.push_front(current_scene);

    if(next_scene != Scene::SceneExit) {
        scenes[current_scene]->on_exit(this);
        current_scene = next_scene;
        scenes[current_scene]->on_enter(this);
    }
}

void iButtonApp::search_and_switch_to_previous_scene(std::initializer_list<Scene> scenes_list) {
    Scene previous_scene = Scene::SceneStart;
    bool scene_found = false;

    while(!scene_found) {
        previous_scene = get_previous_scene();
        for(Scene element : scenes_list) {
            if(previous_scene == element || previous_scene == Scene::SceneStart) {
                scene_found = true;
                break;
            }
        }
    }

    scenes[current_scene]->on_exit(this);
    current_scene = previous_scene;
    scenes[current_scene]->on_enter(this);
}

bool iButtonApp::switch_to_previous_scene(uint8_t count) {
    Scene previous_scene = Scene::SceneStart;

    for(uint8_t i = 0; i < count; i++) {
        previous_scene = get_previous_scene();
        if(previous_scene == Scene::SceneExit) break;
    }

    if(previous_scene == Scene::SceneExit) {
        return true;
    } else {
        scenes[current_scene]->on_exit(this);
        current_scene = previous_scene;
        scenes[current_scene]->on_enter(this);
        return false;
    }
}

iButtonApp::Scene iButtonApp::get_previous_scene() {
    Scene scene = previous_scenes_list.front();
    previous_scenes_list.pop_front();
    return scene;
}

const GpioPin* iButtonApp::get_ibutton_pin() {
    // TODO open record
    return &ibutton_gpio;
}

KeyWorker* iButtonApp::get_key_worker() {
    return key_worker;
}

iButtonKey* iButtonApp::get_key() {
    return &key;
}

SdCard_Api* iButtonApp::get_sd_ex_api() {
    return sd_ex_api;
}

FS_Api* iButtonApp::get_fs_api() {
    return fs_api;
}

char* iButtonApp::get_file_name() {
    return file_name;
}

uint8_t iButtonApp::get_file_name_size() {
    return file_name_size;
}

void iButtonApp::notify_green_blink() {
    notification_message(notification, &sequence_blink_green_10);
}

void iButtonApp::notify_yellow_blink() {
    notification_message(notification, &sequence_blink_yellow_10);
}

void iButtonApp::notify_red_blink() {
    notification_message(notification, &sequence_blink_red_10);
}

void iButtonApp::notify_error() {
    notification_message(notification, &sequence_error);
}

void iButtonApp::notify_success() {
    notification_message(notification, &sequence_success);
}

void iButtonApp::notify_green_on() {
    notification_message_block(notification, &sequence_set_green_255);
}

void iButtonApp::notify_green_off() {
    notification_message(notification, &sequence_reset_green);
}

void iButtonApp::notify_red_on() {
    notification_message_block(notification, &sequence_set_red_255);
}

void iButtonApp::notify_red_off() {
    notification_message(notification, &sequence_reset_red);
}

void iButtonApp::set_text_store(const char* text...) {
    va_list args;
    va_start(args, text);

    vsnprintf(text_store, text_store_size, text, args);

    va_end(args);
}

char* iButtonApp::get_text_store() {
    return text_store;
}

uint8_t iButtonApp::get_text_store_size() {
    return text_store_size;
}

void iButtonApp::generate_random_name(char* name, uint8_t max_name_size) {
    const uint8_t prefix_size = 9;
    const char* prefix[prefix_size] = {
        "ancient",
        "hollow",
        "strange",
        "disappeared",
        "unknown",
        "unthinkable",
        "unnamable",
        "nameless",
        "my",
    };

    const uint8_t suffix_size = 8;
    const char* suffix[suffix_size] = {
        "door",
        "entrance",
        "doorway",
        "entry",
        "portal",
        "entree",
        "opening",
        "crack",
    };

    sniprintf(
        name, max_name_size, "%s_%s", prefix[rand() % prefix_size], suffix[rand() % suffix_size]);
    // to upper
    name[0] = name[0] - 0x20;
}

// file managment
void iButtonApp::show_file_error_message(const char* error_text) {
    set_text_store(error_text);
    get_sd_ex_api()->show_error(get_sd_ex_api()->context, get_text_store());
}

bool iButtonApp::save_key(const char* key_name) {
    File key_file;
    string_t key_file_name;
    bool result = false;
    FS_Error fs_result;
    uint16_t write_count;

    // Create ibutton directory if necessary
    fs_result = get_fs_api()->common.mkdir(app_folder);
    if(fs_result != FSE_OK && fs_result != FSE_EXIST) {
        show_file_error_message("Cannot create\napplication folder");
        return false;
    };

    // First remove key if it was saved
    string_init_set_str(key_file_name, app_folder);
    string_cat_str(key_file_name, "/");
    string_cat_str(key_file_name, get_key()->get_name());
    string_cat_str(key_file_name, app_extension);
    fs_result = get_fs_api()->common.remove(string_get_cstr(key_file_name));
    if(fs_result != FSE_OK && fs_result != FSE_NOT_EXIST) {
        string_clear(key_file_name);
        show_file_error_message("Cannot remove\nold key file");
        return false;
    };

    // Save the key
    get_key()->set_name(key_name);
    string_set_str(key_file_name, app_folder);
    string_cat_str(key_file_name, "/");
    string_cat_str(key_file_name, get_key()->get_name());
    string_cat_str(key_file_name, app_extension);

    bool res = get_fs_api()->file.open(
        &key_file, string_get_cstr(key_file_name), FSAM_WRITE, FSOM_CREATE_ALWAYS);
    string_clear(key_file_name);

    if(res) {
        // type header
        const char* key_type = "E";

        switch(get_key()->get_key_type()) {
        case iButtonKeyType::KeyCyfral:
            key_type = "C";
            break;
        case iButtonKeyType::KeyDallas:
            key_type = "D";
            break;
        case iButtonKeyType::KeyMetakom:
            key_type = "M";
            break;
        }

        write_count = get_fs_api()->file.write(&key_file, key_type, 1);
        if(key_file.error_id != FSE_OK || write_count != 1) {
            show_file_error_message("Cannot write\nto key file");
            get_fs_api()->file.close(&key_file);
            return false;
        }

        const uint8_t byte_text_size = 4;
        char byte_text[byte_text_size];

        for(uint8_t i = 0; i < get_key()->get_type_data_size(); i++) {
            sniprintf(byte_text, byte_text_size, " %02X", get_key()->get_data()[i]);
            write_count = get_fs_api()->file.write(&key_file, byte_text, 3);
            if(key_file.error_id != FSE_OK || write_count != 3) {
                show_file_error_message("Cannot write\nto key file");
                get_fs_api()->file.close(&key_file);
                return false;
            }
        }
        result = true;
    } else {
        show_file_error_message("Cannot create\nnew key file");
    }

    get_fs_api()->file.close(&key_file);
    get_sd_ex_api()->check_error(get_sd_ex_api()->context);

    return result;
}

bool iButtonApp::load_key() {
    bool result = false;

    // Input events and views are managed by file_select
    bool res = get_sd_ex_api()->file_select(
        get_sd_ex_api()->context,
        app_folder,
        app_extension,
        get_file_name(),
        get_file_name_size(),
        get_key()->get_name());

    if(res) {
        string_t key_str;
        File key_file;
        uint16_t read_count;

        // Get key file path
        string_init_set_str(key_str, app_folder);
        string_cat_str(key_str, "/");
        string_cat_str(key_str, get_file_name());
        string_cat_str(key_str, app_extension);

        // Open key file
        get_fs_api()->file.open(
            &key_file, string_get_cstr(key_str), FSAM_READ, FSOM_OPEN_EXISTING);
        string_clear(key_str);

        if(key_file.error_id != FSE_OK) {
            show_file_error_message("Cannot open\nkey file");
            get_fs_api()->file.close(&key_file);
            return false;
        }

        const uint8_t byte_text_size = 4;
        char byte_text[byte_text_size] = {0, 0, 0, 0};

        // load type header
        read_count = get_fs_api()->file.read(&key_file, byte_text, 1);
        if(key_file.error_id != FSE_OK || read_count != 1) {
            show_file_error_message("Cannot read\nkey file");
            get_fs_api()->file.close(&key_file);
            return false;
        }

        iButtonKeyType key_type = iButtonKeyType::KeyCyfral;
        if(strcmp(byte_text, "C") == 0) {
            key_type = iButtonKeyType::KeyCyfral;
        } else if(strcmp(byte_text, "M") == 0) {
            key_type = iButtonKeyType::KeyMetakom;
        } else if(strcmp(byte_text, "D") == 0) {
            key_type = iButtonKeyType::KeyDallas;
        } else {
            show_file_error_message("Cannot parse\nkey file");
            get_fs_api()->file.close(&key_file);
            return false;
        }

        get_key()->set_type(key_type);

        // load data
        uint8_t key_data[IBUTTON_KEY_DATA_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0};
        for(uint8_t i = 0; i < get_key()->get_type_data_size(); i++) {
            // space
            read_count = get_fs_api()->file.read(&key_file, byte_text, 1);
            if(key_file.error_id != FSE_OK || read_count != 1) {
                show_file_error_message("Cannot read\nkey file");
                get_fs_api()->file.close(&key_file);
                return false;
            }

            // value
            read_count = get_fs_api()->file.read(&key_file, byte_text, 2);
            if(key_file.error_id != FSE_OK || read_count != 2) {
                show_file_error_message("Cannot read\nkey file");
                get_fs_api()->file.close(&key_file);
                return false;
            }

            // convert hex value to byte
            key_data[i] = strtol(byte_text, NULL, 16);
        }

        get_fs_api()->file.close(&key_file);

        get_key()->set_name(get_file_name());
        get_key()->set_type(key_type);
        get_key()->set_data(key_data, IBUTTON_KEY_DATA_SIZE);

        result = true;
    }

    get_sd_ex_api()->check_error(get_sd_ex_api()->context);

    return result;
}

bool iButtonApp::delete_key() {
    iButtonKey* key = get_key();
    string_t key_file_name;
    bool result = false;

    string_init_set_str(key_file_name, app_folder);
    string_cat_str(key_file_name, "/");
    string_cat_str(key_file_name, key->get_name());
    string_cat_str(key_file_name, app_extension);
    result = (get_fs_api()->common.remove(string_get_cstr(key_file_name)) == FSE_OK);
    string_clear(key_file_name);

    return result;
}